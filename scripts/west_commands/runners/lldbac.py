# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''ARC architecture-specific debug runner for LLDB (lldbac).

This runner provides debug and debugserver capabilities for ARC targets using
the lldbac debugger. It supports debugging through nsim simulator with GDB 
remote protocol.

For flashing/running applications, use the arc-nsim runner or nsimdrv directly.

Usage Examples:
  Debug with nsim simulator:
    west debug --runner lldbac
  
  Start GDB server for remote debugging:
    west debugserver --runner lldbac --gdb-port 3333
    Then connect: lldbac --file zephyr.elf -o 'gdb-remote localhost:3333'

Configuration:
  --tcf: Tool Configuration File for target configuration
  --gdb-port: GDB server port for nsim connection (default: 3333)
  --nsim-props: Path to nsim properties file (default: rmx100.props)
'''

import os
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner

# Default configuration values
DEFAULT_LLDBAC_GDB_PORT = 3333
DEFAULT_NSIM_PROPS = 'rmx100.props'


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for ARC LLDB (lldbac) debugging.'''

    def __init__(self, cfg, tcf=None, gdb_port=DEFAULT_LLDBAC_GDB_PORT,
                 nsim_props=DEFAULT_NSIM_PROPS):
        super().__init__(cfg)
        self.lldbac_cmd = ['lldbac']
        self.tcf = tcf
        self.gdb_port = gdb_port
        self.nsim_props = nsim_props

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--tcf', 
                           help='TCF (Tool Configuration File) for target configuration')
        parser.add_argument('--gdb-port', type=int, default=DEFAULT_LLDBAC_GDB_PORT,
                           help=f'GDB server port for nsim connection (default: {DEFAULT_LLDBAC_GDB_PORT})')
        parser.add_argument('--nsim-props', default=DEFAULT_NSIM_PROPS,
                           help=f'Path to nsim properties file for debugging (default: {DEFAULT_NSIM_PROPS})')

    @classmethod
    def do_create(cls, cfg, args):
        return LldbacBinaryRunner(
            cfg,
            tcf=args.tcf,
            gdb_port=args.gdb_port,
            nsim_props=args.nsim_props
        )

    def do_run(self, command, **kwargs):
        self.require(self.lldbac_cmd[0])
        
        if command == 'debug':
            self.do_debug(**kwargs)
        elif command == 'debugserver':
            self.do_debugserver(**kwargs)
        else:
            raise ValueError(f'command {command} not supported by {self.name()}')

    def do_debug(self, **kwargs):
        '''Start interactive debugging session with nsim simulator'''
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