# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''ARC architecture-specific runner for LLDB (lldbac).'''

import os
import json
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner

# Default configuration values
DEFAULT_LLDBAC_GDB_PORT = 3333
DEFAULT_FPGA_DEVICE = 'NexysVideo'
DEFAULT_FPGA_INTERFACE = 0
DEFAULT_BOARD_CONFIG = 'board-rmxsdp.json'
DEFAULT_NSIM_PROPS = 'rmx100.props'
DEFAULT_FPGA_PROGRAMMER = 'djtgcfg'

# Supported FPGA programmers and their command formats
FPGA_PROGRAMMERS = {
    'djtgcfg': {
        'cmd': 'djtgcfg',
        'args': ['prog', '-d', '{device}', '-i', '{interface}', '-f', '{bitfile}']
    },
    'confprosh': {
        'cmd': 'confprosh',
        'args': ['-f', '{bitfile}', '-d', '{device}']
    }
}


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for ARC LLDB (lldbac).'''

    def __init__(self, cfg, tcf=None, fpga_program=True, fpga_bitfile=None, 
                 fpga_device=DEFAULT_FPGA_DEVICE, fpga_interface=DEFAULT_FPGA_INTERFACE, 
                 fpga_programmer=DEFAULT_FPGA_PROGRAMMER, board_config=DEFAULT_BOARD_CONFIG, 
                 gdb_port=DEFAULT_LLDBAC_GDB_PORT, nsim_props=DEFAULT_NSIM_PROPS, 
                 hardware_mode=False, simulator_mode=False):
        super().__init__(cfg)
        self.lldbac_cmd = ['lldbac']
        self.tcf = tcf
        self.fpga_program = fpga_program
        self.fpga_bitfile = fpga_bitfile
        self.fpga_device = fpga_device
        self.fpga_interface = fpga_interface
        self.fpga_programmer = fpga_programmer
        self.board_config = board_config
        self.gdb_port = gdb_port
        self.nsim_props = nsim_props
        self.hardware_mode = hardware_mode
        self.simulator_mode = simulator_mode

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--tcf', 
                           help='TCF (Tool Configuration File) for target configuration')
        parser.add_argument('--skip-fpga-program', action='store_true',
                           help='Skip FPGA programming step')
        parser.add_argument('--fpga-bitfile',
                           help='Path to FPGA bitfile (.bit)')
        parser.add_argument('--fpga-device', default=DEFAULT_FPGA_DEVICE,
                           help=f'FPGA device type (default: {DEFAULT_FPGA_DEVICE})')
        parser.add_argument('--fpga-interface', type=int, default=DEFAULT_FPGA_INTERFACE,
                           help=f'FPGA interface index (default: {DEFAULT_FPGA_INTERFACE})')
        parser.add_argument('--fpga-programmer', default=DEFAULT_FPGA_PROGRAMMER,
                           choices=list(FPGA_PROGRAMMERS.keys()),
                           help=f'FPGA programmer tool (default: {DEFAULT_FPGA_PROGRAMMER})')
        parser.add_argument('--board-config', default=DEFAULT_BOARD_CONFIG,
                           help=f'Path to board configuration JSON file (default: {DEFAULT_BOARD_CONFIG})')
        parser.add_argument('--gdb-port', type=int, default=DEFAULT_LLDBAC_GDB_PORT,
                           help=f'GDB server port for nsim connection (default: {DEFAULT_LLDBAC_GDB_PORT})')
        parser.add_argument('--nsim-props', default=DEFAULT_NSIM_PROPS,
                           help=f'Path to nsim properties file for debugging (default: {DEFAULT_NSIM_PROPS})')
        parser.add_argument('--hardware-mode', action='store_true',
                           help='Force hardware mode (FPGA) instead of simulator')
        parser.add_argument('--simulator-mode', action='store_true',
                           help='Force simulator mode (nsim) instead of hardware')

    @classmethod
    def do_create(cls, cfg, args):
        return LldbacBinaryRunner(
            cfg, 
            tcf=args.tcf,
            fpga_program=not args.skip_fpga_program,
            fpga_bitfile=args.fpga_bitfile,
            fpga_device=args.fpga_device,
            fpga_interface=args.fpga_interface,
            fpga_programmer=args.fpga_programmer,
            board_config=args.board_config,
            gdb_port=args.gdb_port,
            nsim_props=args.nsim_props,
            hardware_mode=args.hardware_mode,
            simulator_mode=args.simulator_mode
        )

    def do_run(self, command, **kwargs):
        self.require(self.lldbac_cmd[0])
        
        # Set up board-specific configuration (following nsim pattern)
        if hasattr(self.cfg, 'board_dir') and self.cfg.board_dir:
            kwargs['board-dir'] = self.cfg.board_dir

        if command == 'flash':
            self.do_flash(**kwargs)
        elif command == 'debug':
            self.do_debug(**kwargs)
        elif command == 'debugserver':
            self.do_debugserver(**kwargs)
        else:
            raise ValueError(f'command {command} not supported by {self.name()}')

    def _program_fpga(self):
        '''Program FPGA using specified programmer tool'''
        
        # Get configuration from environment variables or arguments
        fpga_bitfile = (self.fpga_bitfile or 
                       os.environ.get('LLDBAC_FPGA_BITFILE', 
                       'RMX100_featured_NexysV_200T_tar/synopsys_fpga/core_chip.bit'))
        
        fpga_device = (self.fpga_device or 
                      os.environ.get('LLDBAC_FPGA_DEVICE', DEFAULT_FPGA_DEVICE))
        
        fpga_interface = (self.fpga_interface or 
                         int(os.environ.get('LLDBAC_FPGA_INTERFACE', str(DEFAULT_FPGA_INTERFACE))))
        
        fpga_programmer = (self.fpga_programmer or 
                          os.environ.get('LLDBAC_FPGA_PROGRAMMER', DEFAULT_FPGA_PROGRAMMER))
        
        # Validate programmer is supported
        if fpga_programmer not in FPGA_PROGRAMMERS:
            raise ValueError(f"Unsupported FPGA programmer: {fpga_programmer}. "
                           f"Supported: {list(FPGA_PROGRAMMERS.keys())}")
        
        # Get programmer configuration
        programmer_config = FPGA_PROGRAMMERS[fpga_programmer]
        
        # Build command with format substitution
        fpga_cmd = [programmer_config['cmd']]
        for arg in programmer_config['args']:
            formatted_arg = arg.format(
                bitfile=fpga_bitfile,
                device=fpga_device,
                interface=str(fpga_interface)
            )
            fpga_cmd.append(formatted_arg)
        
        self.logger.info(f"Programming FPGA with {fpga_programmer}: {' '.join(fpga_cmd)}")
        self.check_call(fpga_cmd)
    
    def _is_hardware_mode(self):
        '''Determine if we should run in hardware mode or simulator mode'''
        # Explicit hardware mode flag
        if self.hardware_mode:
            return True
            
        # Explicit simulator mode flag  
        if self.simulator_mode:
            return False
        
        # Default to simulator mode (nsim) - simple and predictable
        return False

    def do_flash(self, **kwargs):
        if self._is_hardware_mode():
            self._do_flash_hardware(**kwargs)
        else:
            self._do_flash_simulator(**kwargs)
    
    def _do_flash_hardware(self, **kwargs):
        '''Flash and run on hardware FPGA'''
        # Program FPGA first if enabled
        if self.fpga_program:
            self._program_fpga()
        
        # Get board configuration and build run-lldbac command
        board_config = (self.board_config or 
                       os.environ.get('LLDBAC_BOARD_CONFIG', 'board-rmxsdp.json'))
        
        # Parse board.json and build run-lldbac command
        cmd = self._build_run_lldbac_cmd(board_config, self.cfg.elf_file, run_mode=True)
        
        if self.tcf:
            cmd.extend(['--tcf', self.tcf])

        self.check_call(cmd)
    
    def _do_flash_simulator(self, **kwargs):
        '''Flash and run on nsim simulator'''
        # For simulator mode, use nsim directly
        nsim_props = self._get_nsim_props()
        
        cmd = [
            'nsimdrv',
            '-propsfile', nsim_props,
            self.cfg.elf_file
        ]
        
        self.logger.info(f"Running on nsim: {' '.join(cmd)}")
        self.check_call(cmd)

    def do_debug(self, **kwargs):
        '''Start interactive debugging session'''
        if self._is_hardware_mode():
            self._do_debug_hardware(**kwargs)
        else:
            self._do_debug_simulator(**kwargs)
    
    def _do_debug_hardware(self, **kwargs):
        '''Debug on hardware FPGA'''
        # Program FPGA first if enabled
        if self.fpga_program:
            self._program_fpga()
        
        # Get board configuration and build run-lldbac command
        board_config = (self.board_config or 
                       os.environ.get('LLDBAC_BOARD_CONFIG', 'board-rmxsdp.json'))
        
        # Parse board.json and build run-lldbac command for debug
        cmd = self._build_run_lldbac_cmd(board_config, self.cfg.elf_file, run_mode=False)
        
        if self.tcf:
            cmd.extend(['--tcf', self.tcf])
        
        self.logger.info("Starting interactive debug session on hardware")
        self.check_call(cmd)
    
    def _do_debug_simulator(self, **kwargs):
        '''Debug on nsim simulator'''
        # Check if nsimdrv is available
        try:
            self.require('nsimdrv')
        except Exception:
            self.logger.error("nsimdrv not found. Please install MetaWare nSIM or use hardware mode.")
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
        
        # lldbac debug commands
        lldbac_cmd = self.lldbac_cmd + [
            '--file', self.cfg.elf_file,
            '-o', f'gdb-remote localhost:{self.gdb_port}',
            '-o', 'continue'
        ]

        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])

        # Use run_server_and_client to manage nsim server and lldbac client
        self.run_server_and_client(nsim_server_cmd, lldbac_cmd)

    def do_debugserver(self, **kwargs):
        '''Start nsim as GDB server for remote debugging'''
        # Program FPGA first if enabled and in hardware mode
        if self._is_hardware_mode() and self.fpga_program:
            self._program_fpga()
            
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
    
    def _build_run_lldbac_cmd(self, board_config_path, elf_file, run_mode=True):
        '''Build run-lldbac command from board.json configuration'''
        
        # Start with run-lldbac command
        cmd = ['run-lldbac']
        
        try:
            # Parse board.json configuration
            with open(board_config_path, 'r') as f:
                board_configs = json.load(f)
            
            # Handle single config or array of configs
            if not isinstance(board_configs, list):
                board_configs = [board_configs]
            
            # Use first configuration
            config = board_configs[0]
            
            # Build connection string from board config
            connect_args = []
            
            # Add connect type
            if 'connect' in config:
                connect_args.append(f"connect={config['connect']}")
            
            # Add jtag device
            if 'jtag_device' in config:
                connect_args.append(f"jtag_device={config['jtag_device']}")
            
            # Add jtag frequency  
            if 'jtag_frequency' in config:
                connect_args.append(f"jtag_frequency={config['jtag_frequency']}")
            
            # Add abstract command latency if present
            if 'abstract_command_mem_latency' in config:
                connect_args.append(f"abstract-command-mem-latency={config['abstract_command_mem_latency']}")
            
            # Add connection arguments
            if connect_args:
                cmd.extend(['-c', ' '.join(connect_args)])
            
            # Add ELF file
            cmd.append(elf_file)
            
            # Add postconnect commands
            if 'postconnect' in config:
                for postconnect_cmd in config['postconnect']:
                    cmd.extend(['--postconnect', postconnect_cmd])
            
            # Add run command if in run mode (flash), otherwise just load for debug
            if run_mode:
                cmd.extend(['--postconnect', 'continue'])
                
        except (FileNotFoundError, json.JSONDecodeError, KeyError) as e:
            self.logger.warning(f"Could not parse board config {board_config_path}: {e}")
            self.logger.info("Falling back to simple lldbac command")
            # Fallback to simple lldbac command
            cmd = self.lldbac_cmd + [elf_file]
        
        return cmd