# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import logging
import re
import time

from dataclasses import dataclass, field
from inspect import signature

from twister_harness.device.device_adapter import DeviceAdapter
from twister_harness.exceptions import TwisterHarnessTimeoutException

logger = logging.getLogger(__name__)


class Shell:
    """
    Helper class that provides methods used to interact with shell application.
    """

    def __init__(
        self, device: DeviceAdapter, prompt: str = 'uart:~$', timeout: float | None = None
    ) -> None:
        self._device: DeviceAdapter = device
        self.prompt: str = prompt
        self.base_timeout: float = timeout or device.base_timeout

    def wait_for_prompt(self, timeout: float | None = None) -> bool:
        """
        Send every 0.5 second "enter" command to the device until shell prompt
        statement will occur (return True) or timeout will be exceeded (return
        False).
        
        For very slow boards (IOTDK), wait longer between sends as they can take
        30+ seconds to respond to first input.
        """
        timeout = timeout or self.base_timeout
        timeout_time = time.time() + timeout
        
        # IOTDK is extremely slow - takes 38+ seconds to respond to first newline
        # Send newlines less frequently to give it time to respond
        # Check if this is IOTDK by looking at build dir
        try:
            build_dir_str = str(self._device.device_config.build_dir).lower()
            is_very_slow_board = 'iotdk' in build_dir_str
        except:
            is_very_slow_board = False
        
        # For IOTDK, read any pending data before starting
        # (board might have already output prompt during boot wait)
        if is_very_slow_board:
            try:
                logger.info('Reading any pending boot output...')
                pending = ""
                while True:
                    try:
                        chunk = self._device.readline(timeout=0.1, print_output=False)
                        if chunk:
                            pending += chunk
                            logger.debug(f'Pending data: {chunk[:80]!r}')
                    except TwisterHarnessTimeoutException:
                        break
                if pending:
                    logger.info(f'Found pending output ({len(pending)} chars): {pending[:200]!r}')
                    if self.prompt in pending:
                        logger.info('Prompt already in pending output!')
                        return True
            except Exception as e:
                logger.warning(f'Error reading pending data: {e}')
        
        self._device.clear_buffer()
        
        send_interval = 5.0 if is_very_slow_board else 0.5
        read_timeout = 5.0 if is_very_slow_board else 0.5
        last_send_time = 0
        start_time = time.time()
        newline_count = 0
        accumulated_output = ""  # Accumulate output for IOTDK fragmented responses
        
        logger.info(f'Waiting for prompt (send interval: {send_interval}s, read timeout: {read_timeout}s, timeout: {timeout}s)')
        
        # For IOTDK: Read more frequently (0.5s) but send newlines less frequently (5s)
        # This ensures we don't miss fragmented output
        actual_read_timeout = 0.5 if is_very_slow_board else read_timeout
        
        while time.time() < timeout_time:
            current_time = time.time()
            
            # Send newline at intervals
            if current_time - last_send_time >= send_interval:
                self._device.write(b'\n')
                last_send_time = current_time
                newline_count += 1
                elapsed = current_time - start_time
                logger.debug(f'[{elapsed:.1f}s] Sent newline #{newline_count}')
            
            try:
                line = self._device.readline(timeout=actual_read_timeout, print_output=False)
                # Log any data received
                if line:
                    elapsed = time.time() - start_time
                    accumulated_output += line
                    logger.info(f'[{elapsed:.1f}s] Received: {line[:80]!r}')
                    
                    # Check both current line and accumulated output for prompt
                    # (IOTDK sends data in fragments)
                    if self.prompt in line or self.prompt in accumulated_output:
                        logger.info(f'Got prompt after {elapsed:.1f}s!')
                        if is_very_slow_board:
                            logger.info(f'Accumulated output: {accumulated_output[:200]!r}')
                        return True
            except TwisterHarnessTimeoutException:
                # ignore read timeout and continue checking
                continue
        
        elapsed = time.time() - start_time
        logger.error(f'Prompt not found after {elapsed:.1f}s (sent {newline_count} newlines)')
        if is_very_slow_board and accumulated_output:
            logger.error(f'Accumulated output: {accumulated_output[:200]!r}')
            logger.error(f'Looking for prompt: {self.prompt!r}')
        return False

    def exec_command(
        self, command: str, timeout: float | None = None, print_output: bool = True
    ) -> list[str]:
        """
        Send shell command to a device and return response. Passed command
        is extended by double enter sings - first one to execute this command
        on a device, second one to receive next prompt what is a signal that
        execution was finished. Method returns printout of the executed command.
        """
        timeout = timeout or self.base_timeout
        command_ext = f'{command}\n\n'
        regex_prompt = re.escape(self.prompt)
        regex_command = f'.*{re.escape(command)}'
        self._device.clear_buffer()
        self._device.write(command_ext.encode())
        lines: list[str] = []
        # wait for device command print - it should be done immediately after sending command to device
        lines.extend(
            self._device.readlines_until(
                regex=regex_command, timeout=1.0, print_output=print_output
            )
        )
        # wait for device command execution
        lines.extend(
            self._device.readlines_until(
                regex=regex_prompt, timeout=timeout, print_output=print_output
            )
        )
        return lines

    def get_filtered_output(self, command_lines: list[str]) -> list[str]:
        """
        Filter out prompts and log messages

        Take the output of exec_command, which can contain log messages and command prompts,
        and filter them to obtain only the command output.

        Example:
            >>> # equivalent to `lines = shell.exec_command("kernel version")`
            >>> lines = [
            >>>    'uart:~$',                    # filter prompts
            >>>    'Zephyr version 3.6.0',       # keep this line
            >>>    'uart:~$ <dbg> debug message' # filter log messages
            >>> ]
            >>> filtered_output = shell.get_filtered_output(output)
            >>> filtered_output
            ['Zephyr version 3.6.0']

        :param command_lines: List of strings i.e. the output of `exec_command`.
        :return: A list of strings containing, excluding prompts and log messages.
        """
        regex_filter = re.compile(
            '|'.join([re.escape(self.prompt), '<dbg>', '<inf>', '<wrn>', '<err>'])
        )
        return list(filter(lambda l: not regex_filter.search(l), command_lines))


@dataclass
class ShellMCUbootArea:
    name: str
    version: str
    image_size: str
    magic: str = 'unset'
    swap_type: str = 'none'
    copy_done: str = 'unset'
    image_ok: str = 'unset'

    @classmethod
    def from_kwargs(cls, **kwargs) -> ShellMCUbootArea:
        cls_fields = {field for field in signature(cls).parameters}
        native_args = {}
        for name, val in kwargs.items():
            if name in cls_fields:
                native_args[name] = val
        return cls(**native_args)


@dataclass
class ShellMCUbootCommandParsed:
    """
    Helper class to keep data from `mcuboot` shell command.
    """

    areas: list[ShellMCUbootArea] = field(default_factory=list)

    @classmethod
    def create_from_cmd_output(cls, cmd_output: list[str]) -> ShellMCUbootCommandParsed:
        """
        Factory to create class from the output of `mcuboot` shell command.
        """
        areas: list[dict] = []
        re_area = re.compile(r'(.+ area.*):\s*$')
        re_key = re.compile(r'(?P<key>.+):(?P<val>.+)')
        for line in cmd_output:
            if m := re_area.search(line):
                areas.append({'name': m.group(1)})
            elif areas:
                if m := re_key.search(line):
                    areas[-1][m.group('key').strip().replace(' ', '_')] = m.group('val').strip()
        data_areas: list[ShellMCUbootArea] = []
        for area in areas:
            data_areas.append(ShellMCUbootArea.from_kwargs(**area))

        return cls(data_areas)
