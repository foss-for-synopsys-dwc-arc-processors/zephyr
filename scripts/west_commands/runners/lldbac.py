# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''ARC architecture-specific runner for LLDB (lldbac).

This runner provides flash, debug and debugserver capabilities for ARC targets
on both simulator (nsim) and physical hardware.

Usage Examples:
  Simulator mode (default):
    west flash --runner lldbac                    # Run on nsim
    west debug --runner lldbac                    # Debug on nsim
  
  Hardware mode:
    west flash --runner lldbac --hardware         # Flash and run on hardware
    west debug --runner lldbac --hardware         # Debug on hardware
  
  Start GDB server (simulator):
    west debugserver --runner lldbac --gdb-port 3333
    Then connect: lldbac --file zephyr.elf -o 'gdb-remote localhost:3333'

Configuration:
  --hardware: Use physical hardware instead of simulator
  --tcf: Tool Configuration File for target configuration
  --gdb-port: GDB server port for nsim connection (default: 3333)  
  --nsim-props: Path to nsim properties file (default: rmx100.props)
  --jtag-device: JTAG device for hardware connection (default: USB0)
  --jtag-frequency: JTAG clock frequency in Hz (default: 10000000)
'''

import os
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner

# Default configuration values
DEFAULT_LLDBAC_GDB_PORT = 3333
DEFAULT_NSIM_PROPS = 'rmx100.props'


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for ARC LLDB (lldbac) for simulator and hardware.'''

    def __init__(self, cfg, tcf=None, gdb_port=DEFAULT_LLDBAC_GDB_PORT,
                 nsim_props=DEFAULT_NSIM_PROPS, hardware=False,
                 jtag_device='USB0', jtag_frequency=10000000):
        super().__init__(cfg)
        self.lldbac_cmd = ['lldbac']
        self.tcf = tcf
        self.gdb_port = gdb_port
        self.nsim_props = nsim_props
        self.hardware = hardware
        self.jtag_device = jtag_device
        self.jtag_frequency = jtag_frequency

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--hardware', action='store_true',
                           help='Use physical hardware instead of simulator')
        parser.add_argument('--tcf', 
                           help='TCF (Tool Configuration File) for target configuration')
        parser.add_argument('--gdb-port', type=int, default=DEFAULT_LLDBAC_GDB_PORT,
                           help=f'GDB server port for nsim connection (default: {DEFAULT_LLDBAC_GDB_PORT})')
        parser.add_argument('--nsim-props', default=DEFAULT_NSIM_PROPS,
                           help=f'Path to nsim properties file for debugging (default: {DEFAULT_NSIM_PROPS})')
        parser.add_argument('--jtag-device', default='USB0',
                           help='JTAG device for hardware connection (default: USB0)')
        parser.add_argument('--jtag-frequency', type=int, default=10000000,
                           help='JTAG clock frequency in Hz (default: 10000000)')

    @classmethod
    def do_create(cls, cfg, args):
        return LldbacBinaryRunner(
            cfg,
            tcf=args.tcf,
            gdb_port=args.gdb_port,
            nsim_props=args.nsim_props,
            hardware=args.hardware,
            jtag_device=args.jtag_device,
            jtag_frequency=args.jtag_frequency
        )

    def do_run(self, command, **kwargs):
        # For flash, we only need nsimdrv, not lldbac
        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.require(self.lldbac_cmd[0])
            self.do_debug(**kwargs)
        elif command == 'debugserver':
            self.require(self.lldbac_cmd[0])
            self.do_debugserver(**kwargs)
        else:
            raise ValueError(f'command {command} not supported by {self.name()}')
    
    def do_flash(self, **kwargs):
        '''Flash and run on simulator or hardware'''
        if self.hardware:
            self._do_flash_hardware(**kwargs)
        else:
            self._do_flash_simulator(**kwargs)
    
    def _do_flash_simulator(self, **kwargs):
        '''Run on nsim simulator (no debugging)'''
        # Check if nsimdrv is available
        try:
            self.require('nsimdrv')
        except Exception:
            self.logger.error("nsimdrv not found. Please install MetaWare nSIM.")
            raise
        
        # Get nsim props file
        nsim_props = self._get_nsim_props()
        
        # Simply run the application on nsim (no debug, just execute)
        cmd = ['nsimdrv', '-propsfile', nsim_props, self.cfg.elf_file]
        
        self.logger.info(f"Running on nsim: {' '.join(cmd)}")
        self.check_call(cmd)
    
    def _do_flash_hardware(self, **kwargs):
        '''Flash and run on physical hardware using lldbac'''
        self.require(self.lldbac_cmd[0])
        
        # Create lldbac script for hardware flash
        lldbac_cmds = [
            self.lldbac_cmd[0],
            '--file', self.cfg.elf_file,
            '--batch'  # Exit after executing commands
        ]
        
        # Add connection and load commands
        lldbac_cmds.extend([
            '-o', f'platform select remote-gdb-server',
            '-o', f'platform connect connect://jtag:{self.jtag_device}?frequency={self.jtag_frequency}',
            '-o', 'target modules load --load --slide 0',
            '-o', 'process launch --stop-at-entry=false',
            '-o', 'process continue'
        ])
        
        if self.tcf:
            lldbac_cmds.extend(['--tcf', self.tcf])
        
        self.logger.info(f"Flashing to hardware via JTAG device {self.jtag_device}")
        self.check_call(lldbac_cmds)

    def do_debug(self, **kwargs):
        '''Start interactive debugging session'''
        if self.hardware:
            self._do_debug_hardware(**kwargs)
        else:
            self._do_debug_simulator(**kwargs)
    
    def _do_debug_simulator(self, **kwargs):
        '''Debug on nsim simulator'''
        # Check if nsimdrv is available
        try:
            self.require('nsimdrv')
        except Exception:
            self.logger.error("nsimdrv not found. Please install MetaWare nSIM.")
            raise
        
        # Get nsim props file
        nsim_props = self._get_nsim_props()
        
        # Start nsim as GDB server in background
        nsim_server_cmd = [
            'nsimdrv', 
            '-gdb', 
            f'-port={self.gdb_port}',
            '-propsfile', nsim_props
        ]
        
        # lldbac debug commands (no auto-load/continue; user controls session)
        lldbac_cmd = self.lldbac_cmd + [
            '--file', self.cfg.elf_file,
            '-o', f'gdb-remote localhost:{self.gdb_port}'
        ]

        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])

        # Launch nsim GDB server detached from this TTY to avoid console conflicts
        import subprocess
        import time
        
        server_proc = self.popen_ignore_int(
            nsim_server_cmd,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )

        # Give the server a brief moment to start listening
        time.sleep(0.2)

        try:
            # Run lldbac client interactively with normal TTY and SIGINT handling
            self.run_client(lldbac_cmd)
        finally:
            server_proc.terminate()
            server_proc.wait()
    
    def _do_debug_hardware(self, **kwargs):
        '''Debug on physical hardware'''
        self.require(self.lldbac_cmd[0])
        
        # Interactive debug session on hardware
        lldbac_cmd = self.lldbac_cmd + [
            '--file', self.cfg.elf_file,
            '-o', f'platform select remote-gdb-server',
            '-o', f'platform connect connect://jtag:{self.jtag_device}?frequency={self.jtag_frequency}',
            '-o', 'target modules load --load --slide 0',
            '-o', 'target stop'  # Stop for debugging
        ]
        
        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])
        
        self.logger.info(f"Starting debug session on hardware via JTAG device {self.jtag_device}")
        # Run interactively
        self.run_client(lldbac_cmd)

    def do_debugserver(self, **kwargs):
        '''Start nsim as GDB server for remote debugging'''
        # Check if nsimdrv is available
        try:
            self.require('nsimdrv')
        except Exception:
            self.logger.error("nsimdrv not found. Please install MetaWare nSIM.")
            raise
            
        # Get nsim props file
        nsim_props = self._get_nsim_props()
        
        # Start nsim as GDB server
        cmd = [
            'nsimdrv',
            '-gdb',
            f'-port={self.gdb_port}', 
            '-propsfile', nsim_props
        ]
        
        self.logger.info(f"Starting nsim GDB server on port {self.gdb_port}")
        self.logger.info(f"Connect with: lldbac --file {self.cfg.elf_file} -o 'gdb-remote localhost:{self.gdb_port}'")
        self.check_call(cmd)
        
    def _get_nsim_props(self):
        '''Get nsim properties file path following board support directory pattern'''
        # Check if absolute path provided
        if self.nsim_props and path.isabs(self.nsim_props):
            return self.nsim_props
            
        # Look in board support directory (following nsim pattern)
        if hasattr(self.cfg, 'board_dir') and self.cfg.board_dir:
            board_props = path.join(self.cfg.board_dir, 'support', self.nsim_props)
            if path.exists(board_props):
                return board_props
                    
        # Fallback to current directory or environment
        if path.exists(self.nsim_props):
            return self.nsim_props
            
        return os.environ.get('NSIM_PROPS', self.nsim_props)