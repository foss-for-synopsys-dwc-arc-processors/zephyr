# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''LLDB (lldbac) runner.

This runner provides `flash`, `debug` and `debugserver` capabilities for
targets on both simulator (nSIM) and physical hardware.

Usage Examples:
  Simulator mode (default):
    west flash --runner lldbac                    # Run on nsim
    west debug --runner lldbac                    # Debug on nsim
  
  Hardware mode:
    west flash --runner lldbac --hardware         # Flash on hardware
    west debug --runner lldbac --hardware         # Debug on hardware
  
  Start GDB server (simulator):
    west debugserver --runner lldbac --gdb-port 3333
    Then connect: lldbac --file zephyr.elf -o 'gdb-remote localhost:3333'

Configuration:
  --hardware: Use physical hardware instead of simulator
  --tcf: Tool Configuration File for target configuration
  --gdb-port: GDB server port for nsim connection (default: 3333)  
  --nsim-props: nsim properties filename (board-specific, no default)
  --board-json: board.json file for hardware connection (auto-detected from board directory)

Hardware Mode:
  Hardware mode requires a board.json file containing JTAG configuration.
  The runner automatically looks for board.json in the board directory,
  or you can specify a custom path with --board-json.
  
  Expected board.json format:
  [
    {
      "connect": "jtag-digilent",
      "jtag_device": "JtagHs2",
      "jtag_frequency": "500KHz",
      "postconnect": ["command source preload.cmd"]  // optional
      //you can add any lldbac compatible parameters here
    }
  ]

Board Configuration:
  Each board should specify its nsim properties file in board.cmake:
    board_runner_args(lldbac "--nsim-props=board_specific.props")
  
  For hardware support, create board.json in the board directory with
  appropriate JTAG configuration for that board.
'''

import os
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner

# Default configuration values
DEFAULT_LLDBAC_GDB_PORT = 3333


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for LLDB (lldbac) for simulator and hardware.'''

    def __init__(self, cfg, props, tcf=None, gdb_port=DEFAULT_LLDBAC_GDB_PORT,
                 hardware=False, board_json=None):
        super().__init__(cfg)
        self.lldbac_cmd = ['lldbac']
        self.tcf = tcf
        self.gdb_port = gdb_port
        self.props = props
        self.hardware = hardware
        self.board_json = board_json

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
        parser.add_argument('--nsim-props', 
                           help='nsim props filename (looked up under board support)')
        parser.add_argument('--board-json', 
                           help='board.json file for hardware connection (optional, auto-detected from board directory)')

    @classmethod
    def do_create(cls, cfg, args):
        chosen_props = args.nsim_props
        
        # Get board.json for hardware mode
        board_json = None
        if args.hardware:
            board_json = cls._get_board_json(cfg, args)
            
        return LldbacBinaryRunner(
            cfg,
            props=chosen_props,
            tcf=args.tcf,
            gdb_port=args.gdb_port,
            hardware=args.hardware,
            board_json=board_json
        )

    @classmethod
    def _get_board_json(cls, cfg, args):
        '''Get board.json file path for hardware mode'''
        # Use explicit --board-json argument if provided
        if args.board_json:
            if path.isabs(args.board_json):
                board_json_path = args.board_json
            else:
                board_json_path = path.join(cfg.board_dir, args.board_json)
        else:
            # Auto-detect board.json in board directory
            board_json_path = path.join(cfg.board_dir, 'board.json')
        
        # Verify the file exists
        if not path.exists(board_json_path):
            raise ValueError(
                f"Hardware mode requires board.json file. "
                f"Expected at: {board_json_path} or specify with --board-json"
            )
            
        return board_json_path

    def do_run(self, command, **kwargs):
        # Log mode selection to help diagnose environment issues
        self.logger.info(f"lldbac: mode={'hardware' if self.hardware else 'sim'} gdb_port={self.gdb_port} props={self.props}")
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
        
        # Add board.json connection command
        lldbac_cmds.extend([
            '-o', f'platform connect {self.board_json}',
            '-o', 'process launch --stop-at-entry=false',
            '-o', 'process continue'
        ])
        
        if self.tcf:
            lldbac_cmds.extend(['--tcf', self.tcf])
        
        self.logger.info(f"Flashing to hardware using board.json: {self.board_json}")
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
        self.logger.info(f"lldbac: using nsim props: {nsim_props}")
        
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

        # Launch nsim GDB server with stdin detached to avoid TTY stop,
        # stdout/stderr attached for UART output
        import subprocess
        import time

        server_proc = self.popen_ignore_int(
            nsim_server_cmd,
            stdin=subprocess.DEVNULL,
        )

        # Give the server a brief moment to start listening
        time.sleep(0.2)

        try:
            # Run lldbac client interactively and auto-load after connect
            self.run_client(lldbac_cmd + ['-o', 'load'])
        finally:
            server_proc.terminate()
            server_proc.wait()
    
    def _do_debug_hardware(self, **kwargs):
        '''Debug on physical hardware'''
        self.require(self.lldbac_cmd[0])
        
        # Interactive debug session on hardware
        lldbac_cmd = self.lldbac_cmd + [
            '--file', self.cfg.elf_file,
            '-o', f'platform connect {self.board_json}',
            '-o', 'process launch --stop-at-entry=true'  # Stop for debugging
        ]
        
        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])
        
        self.logger.info(f"Starting debug session on hardware using board.json: {self.board_json}")
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
        '''Get nSIM properties file path: board_dir/support/<props>.'''
        if not self.props:
            raise ValueError("nsim-props filename is required but not provided by board configuration")
        
        # Check if absolute path provided
        if path.isabs(self.props):
            return self.props
            
        # Look in board support directory 
        if hasattr(self.cfg, 'board_dir') and self.cfg.board_dir:
            board_props = path.join(self.cfg.board_dir, 'support', self.props)
            if path.exists(board_props):
                return board_props
                    
        # Fallback to current directory
        if path.exists(self.props):
            return self.props
            
        raise ValueError(f"Could not find nsim props file: {self.props}")
