# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import os
if os.name != 'nt':
    import pty
import re
import subprocess
import time
from pathlib import Path

import serial
from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.exceptions import (
    TwisterHarnessException,
    TwisterHarnessTimeoutException,
)
from twister_harness.device.utils import log_command, terminate_process
from twister_harness.twister_harness_config import DeviceConfig

logger = logging.getLogger(__name__)


class HardwareAdapter(DeviceAdapter):
    """Adapter class for real device."""

    def __init__(self, device_config: DeviceConfig) -> None:
        super().__init__(device_config)
        self._flashing_timeout: float = device_config.flash_timeout
        self._serial_connection: serial.Serial | None = None
        self._serial_pty_proc: subprocess.Popen | None = None
        self._serial_buffer: bytearray = bytearray()

        self.device_log_path: Path = device_config.build_dir / 'device.log'
        self._log_files.append(self.device_log_path)

    def _generate_flash_command(self) -> None:
        command = [self.device_config.flash_command[0]]
        command.extend(['--build-dir', str(self.device_config.build_dir)])

        if self.device_config.id:
            command.extend(['--board-id', self.device_config.id])

        command.extend(self.device_config.flash_command[1:])

        self.command = command

    def generate_command(self) -> None:
        """Return command to flash."""
        if self.device_config.flash_command:
            self._generate_flash_command()
            return

        command = [
            self.west,
            'flash',
            '--skip-rebuild',
            '--build-dir', str(self.device_config.build_dir),
        ]

        command_extra_args = []
        if self.device_config.west_flash_extra_args:
            command_extra_args.extend(self.device_config.west_flash_extra_args)

        if self.device_config.runner:
            runner_base_args, runner_extra_args = self._prepare_runner_args()
            command.extend(runner_base_args)
            command_extra_args.extend(runner_extra_args)

        if command_extra_args:
            command.append('--')
            command.extend(command_extra_args)
        self.command = command

    def _prepare_runner_args(self) -> tuple[list[str], list[str]]:
        base_args: list[str] = []
        extra_args: list[str] = []
        runner = self.device_config.runner
        base_args.extend(['--runner', runner])
        if self.device_config.runner_params:
            for param in self.device_config.runner_params:
                extra_args.append(param)
        if board_id := self.device_config.id:
            if runner == 'pyocd':
                extra_args.append('--board-id')
                extra_args.append(board_id)
            elif runner == "esp32":
                extra_args.append("--esp-device")
                extra_args.append(board_id)
            elif runner in ('nrfjprog', 'nrfutil', 'nrfutil_next'):
                extra_args.append('--dev-id')
                extra_args.append(board_id)
            elif runner == 'openocd' and self.device_config.product in ['STM32 STLink', 'STLINK-V3']:
                extra_args.append('--cmd-pre-init')
                extra_args.append(f'hla_serial {board_id}')
            elif runner == 'openocd' and self.device_config.product == 'EDBG CMSIS-DAP':
                extra_args.append('--cmd-pre-init')
                extra_args.append(f'cmsis_dap_serial {board_id}')
            elif runner == "openocd" and self.device_config.product == "LPC-LINK2 CMSIS-DAP":
                extra_args.append("--cmd-pre-init")
                extra_args.append(f'adapter serial {board_id}')
            elif runner == 'jlink':
                base_args.append('--dev-id')
                base_args.append(board_id)
            elif runner == 'stm32cubeprogrammer' and self.device_config.product != "BOOT-SERIAL":
                base_args.append(f'--tool-opt=sn={board_id}')
            elif runner == 'linkserver':
                base_args.append(f'--probe={board_id}')
        return base_args, extra_args

    def _flash_and_run(self) -> None:
        """Flash application on a device."""
        if not self.command:
            msg = 'Flash command is empty, please verify if it was generated properly.'
            logger.error(msg)
            raise TwisterHarnessException(msg)

        if self.device_config.pre_script:
            self._run_custom_script(self.device_config.pre_script, self.base_timeout)

        if self.device_config.id:
            logger.debug('Flashing device %s', self.device_config.id)
        log_command(logger, 'Flashing command', self.command, level=logging.DEBUG)

        process = stdout = None
        try:
            process = subprocess.Popen(self.command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=self.env)
            stdout, _ = process.communicate(timeout=self._flashing_timeout)
        except subprocess.TimeoutExpired as exc:
            process.kill()
            msg = f'Timeout occurred ({self._flashing_timeout}s) during flashing.'
            logger.error(msg)
            raise TwisterHarnessTimeoutException(msg) from exc
        except subprocess.SubprocessError as exc:
            msg = f'Flashing subprocess failed due to SubprocessError {exc}'
            logger.error(msg)
            raise TwisterHarnessTimeoutException(msg) from exc
        finally:
            if stdout is not None:
                stdout_decoded = stdout.decode(errors='ignore')
                with open(self.device_log_path, 'a+') as log_file:
                    log_file.write(stdout_decoded)
            if self.device_config.post_flash_script:
                self._run_custom_script(self.device_config.post_flash_script, self.base_timeout)
            if process is not None and process.returncode == 0:
                logger.debug('Flashing finished')
                # Check board type first
                build_dir_str = str(self.device_config.build_dir).lower()
                is_iotdk = 'iotdk' in build_dir_str
                
                # IOTDK: Don't reconnect - USB may not reset or serial stays valid
                # Diagnostic shows NO data when reconnecting, suggesting serial is dead
                if is_iotdk:
                    logger.info('IOTDK detected - keeping existing serial connection')
                    logger.info('Clearing buffers and waiting for boot')
                    if self._serial_connection and self._serial_connection.is_open:
                        self._serial_connection.reset_input_buffer()
                        self._serial_connection.reset_output_buffer()
                    # Wait for device to boot after flash
                    boot_wait = 60.0
                    logger.info(f'Waiting for device boot ({boot_wait}s)')
                    time.sleep(boot_wait)
                    logger.info('Ready to detect prompt')
                # Other boards: Reconnect serial after flash (USB device was reset during flashing)
                elif self._serial_connection and self._serial_connection.is_open:
                    logger.info('Closing serial after flash')
                    self._serial_connection.close()
                    usb_wait = 2.0
                    logger.info(f'Waiting for USB to stabilize ({usb_wait}s)')
                    time.sleep(usb_wait)
                    logger.info('Reconnecting serial')
                    self._connect_device()
                    # ARC boards need substantial boot time after USB reset
                    boot_wait = 20.0
                    logger.info(f'Waiting for device boot ({boot_wait}s)')
                    time.sleep(boot_wait)
                    logger.info('Ready to detect prompt')
            else:
                msg = f'Could not flash device {self.device_config.id}'
                logger.error(msg)
                raise TwisterHarnessException(msg)

    def _connect_device(self) -> None:
        serial_name = self._open_serial_pty() or self.device_config.serial
        logger.debug('Opening serial connection for %s', serial_name)
        
        # Check if device exists
        if serial_name and serial_name.startswith('/dev/'):
            if os.path.exists(serial_name):
                logger.debug(f'Serial device {serial_name} exists')
            else:
                logger.error(f'Serial device {serial_name} does NOT exist!')
        
        try:
            self._serial_connection = serial.Serial(
                serial_name,
                baudrate=self.device_config.baud,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                bytesize=serial.EIGHTBITS,
                timeout=self.base_timeout,
            )
        except serial.SerialException as exc:
            logger.exception('Cannot open connection: %s', exc)
            self._close_serial_pty()
            raise

        logger.debug(f'Serial connection opened successfully: DTR={self._serial_connection.dtr}, RTS={self._serial_connection.rts}')
        self._serial_connection.flush()
        self._serial_connection.reset_input_buffer()
        self._serial_connection.reset_output_buffer()

    def _diagnose_iotdk_boot(self) -> None:
        """
        Diagnostic function for IOTDK to understand boot timing and communication
        This runs immediately after reconnection to capture real boot behavior
        """
        logger.info('Starting IOTDK boot diagnostic (60 seconds)...')
        logger.info('This will help identify the actual boot timing')
        
        # DON'T reset - board should already be booting from flash
        # Try hardware reset via DTR/RTS signals - DISABLED as it might interrupt boot
        # logger.info('DIAGNOSTIC: Attempting hardware reset via DTR/RTS...')
        # try:
        #     self._serial_connection.dtr = False
        #     self._serial_connection.rts = False
        #     time.sleep(0.1)
        #     self._serial_connection.dtr = True
        #     self._serial_connection.rts = True
        #     time.sleep(0.5)
        #     logger.info('DIAGNOSTIC: DTR/RTS toggle complete')
        # except Exception as e:
        #     logger.warning(f'DIAGNOSTIC: DTR/RTS toggle failed: {e}')
        
        # DON'T send break - might interrupt boot
        # Try sending break signal
        # logger.info('DIAGNOSTIC: Attempting break signal...')
        # try:
        #     self._serial_connection.send_break(duration=0.25)
        #     time.sleep(0.5)
        #     logger.info('DIAGNOSTIC: Break signal sent')
        # except Exception as e:
        #     logger.warning(f'DIAGNOSTIC: Break signal failed: {e}')
        
        # Check serial port parameters
        logger.info(f'DIAGNOSTIC: Serial port: {self._serial_connection.port}')
        logger.info(f'DIAGNOSTIC: Baud rate: {self._serial_connection.baudrate}')
        logger.info(f'DIAGNOSTIC: Is open: {self._serial_connection.is_open}')
        logger.info(f'DIAGNOSTIC: DTR: {self._serial_connection.dtr}, RTS: {self._serial_connection.rts}')
        
        start_time = time.time()
        all_data = bytearray()
        first_data_time = None
        prompt_found_time = None
        data_chunks = []
        
        # Just listen passively for boot messages - don't send anything
        # Listen for 60 seconds to capture boot sequence
        while time.time() - start_time < 60:
            if self._serial_connection.in_waiting > 0:
                chunk_time = time.time() - start_time
                data = self._serial_connection.read(self._serial_connection.in_waiting)
                
                if first_data_time is None:
                    first_data_time = chunk_time
                    logger.info(f'DIAGNOSTIC: First data received at {first_data_time:.1f}s')
                
                all_data.extend(data)
                data_chunks.append((chunk_time, len(data)))
                
                # Log every data arrival
                logger.info(f'DIAGNOSTIC: [{chunk_time:6.1f}s] +{len(data):4d} bytes (total: {len(all_data):5d})')
                
                # Check if prompt appeared
                if prompt_found_time is None and b'uart:~$' in all_data:
                    prompt_found_time = chunk_time
                    logger.info(f'DIAGNOSTIC: *** PROMPT FOUND at {prompt_found_time:.1f}s! ***')
            
            # Don't send anything - just listen passively
            elapsed = time.time() - start_time
            
            # Log progress every 10 seconds
            if int(elapsed) % 10 == 0 and elapsed - int(elapsed) < 0.1:
                logger.info(f'DIAGNOSTIC: [{elapsed:6.1f}s] Waiting... (total bytes: {len(all_data)})')
            time.sleep(0.1)  # Check more frequently
        
        # Summary
        logger.info('=' * 80)
        logger.info('IOTDK DIAGNOSTIC SUMMARY:')
        logger.info(f'  Total time: 60.0s')
        logger.info(f'  Total data received: {len(all_data)} bytes')
        logger.info(f'  First data at: {first_data_time if first_data_time else "NEVER"}s')
        logger.info(f'  Prompt found at: {prompt_found_time if prompt_found_time else "NOT FOUND"}')
        logger.info(f'  Number of data chunks: {len(data_chunks)}')
        
        if all_data:
            # Show all data with hex representation for analysis
            logger.info(f'  All data (hex): {all_data.hex(" ")}')
            logger.info(f'  All data (repr): {repr(bytes(all_data))}')
            logger.info(f'  All data (ascii, errors=ignore): {all_data.decode("ascii", errors="ignore")}')
            
            # Check for various patterns
            patterns_to_check = [
                (b'uart:~$', 'Full prompt'),
                (b'uart:', 'Prompt prefix'),
                (b'~$', 'Prompt suffix'),
                (b'help', 'Help response'),
                (b'kernel', 'Kernel command'),
                (b'Unknown command', 'Unknown command response'),
                (b'Zephyr', 'Zephyr banner'),
                (b'\x1b[', 'ANSI escape'),
            ]
            
            for pattern, desc in patterns_to_check:
                if pattern in all_data:
                    pos = all_data.find(pattern)
                    logger.info(f'  Found "{desc}" at byte {pos}')
            
            # Check for prompt pattern
            if b'uart:~$' in all_data:
                prompt_pos = all_data.find(b'uart:~$')
                logger.info(f'  Prompt position: byte {prompt_pos}')
                context_start = max(0, prompt_pos - 50)
                context_end = min(len(all_data), prompt_pos + 50)
                logger.info(f'  Prompt context: {repr(bytes(all_data[context_start:context_end]))}')
            else:
                logger.warning('  WARNING: Prompt pattern "uart:~$" NOT found!')
            
            # Save diagnostic output
            diag_file = self.device_config.build_dir / 'iotdk_diagnostic.bin'
            try:
                with open(diag_file, 'wb') as f:
                    f.write(all_data)
                logger.info(f'  Diagnostic data saved to: {diag_file}')
            except Exception as e:
                logger.warning(f'  Could not save diagnostic data: {e}')
        else:
            logger.error('  ERROR: NO DATA RECEIVED AT ALL!')
        
        logger.info('=' * 80)
        logger.info('DIAGNOSTIC COMPLETE - Continuing with normal boot wait...')

    def _open_serial_pty(self) -> str | None:
        """Open a pty pair, run process and return tty name"""
        if not self.device_config.serial_pty:
            return None

        try:
            master, slave = pty.openpty()
        except NameError as exc:
            logger.exception('PTY module is not available.')
            raise exc

        try:
            self._serial_pty_proc = subprocess.Popen(
                re.split(',| ', self.device_config.serial_pty),
                stdout=master,
                stdin=master,
                stderr=master
            )
        except subprocess.CalledProcessError as exc:
            logger.exception('Failed to run subprocess %s, error %s', self.device_config.serial_pty, str(exc))
            raise
        return os.ttyname(slave)

    def _disconnect_device(self) -> None:
        if self._serial_connection:
            serial_name = self._serial_connection.port
            self._serial_connection.close()
            # self._serial_connection = None
            logger.debug('Closed serial connection for %s', serial_name)
        self._close_serial_pty()

    def _close_serial_pty(self) -> None:
        """Terminate the process opened for serial pty script"""
        if self._serial_pty_proc:
            self._serial_pty_proc.terminate()
            self._serial_pty_proc.communicate(timeout=self.base_timeout)
            logger.debug('Process %s terminated', self.device_config.serial_pty)
            self._serial_pty_proc = None

    def _close_device(self) -> None:
        if self.device_config.post_script:
            self._run_custom_script(self.device_config.post_script, self.base_timeout)

    def is_device_running(self) -> bool:
        return self._device_run.is_set()

    def is_device_connected(self) -> bool:
        return bool(
            self.is_device_running()
            and self._device_connected.is_set()
            and self._serial_connection
            and self._serial_connection.is_open
        )

    def _read_device_output(self) -> bytes:
        try:
            output = self._readline_serial()
        except (serial.SerialException, TypeError, IOError):
            # serial was probably disconnected
            output = b''
        return output

    def _readline_serial(self) -> bytes:
        """
        This method was created to avoid using PySerial built-in readline
        method which cause blocking reader thread even if there is no data to
        read. Instead for this, following implementation try to read data only
        if they are available. Inspiration for this code was taken from this
        comment:
        https://github.com/pyserial/pyserial/issues/216#issuecomment-369414522
        """
        line = self._readline_from_serial_buffer()
        if line is not None:
            return line
        while True:
            if self._serial_connection is None or not self._serial_connection.is_open:
                return b''
            elif self._serial_connection.in_waiting == 0:
                time.sleep(0.05)
                continue
            else:
                bytes_to_read = max(1, min(2048, self._serial_connection.in_waiting))
                output = self._serial_connection.read(bytes_to_read)
                self._serial_buffer.extend(output)
                line = self._readline_from_serial_buffer()
                if line is not None:
                    return line

    def _readline_from_serial_buffer(self) -> bytes | None:
        idx = self._serial_buffer.find(b"\n")
        if idx >= 0:
            line = self._serial_buffer[:idx+1]
            self._serial_buffer = self._serial_buffer[idx+1:]
            return bytes(line)
        else:
            return None

    def _write_to_device(self, data: bytes) -> None:
        self._serial_connection.write(data)

    def _flush_device_output(self) -> None:
        if self.is_device_connected():
            self._serial_connection.flush()
            self._serial_connection.reset_input_buffer()

    def _clear_internal_resources(self) -> None:
        super()._clear_internal_resources()
        self._serial_connection = None
        self._serial_pty_proc = None
        self._serial_buffer.clear()

    @staticmethod
    def _run_custom_script(script_path: str | Path, timeout: float) -> None:
        with subprocess.Popen(str(script_path), stderr=subprocess.PIPE, stdout=subprocess.PIPE) as proc:
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
                logger.debug(stdout.decode())
                if proc.returncode != 0:
                    msg = f'Custom script failure: \n{stderr.decode(errors="ignore")}'
                    logger.error(msg)
                    raise TwisterHarnessException(msg)

            except subprocess.TimeoutExpired as exc:
                terminate_process(proc)
                proc.communicate(timeout=timeout)
                msg = f'Timeout occurred ({timeout}s) during execution custom script: {script_path}'
                logger.error(msg)
                raise TwisterHarnessTimeoutException(msg) from exc
