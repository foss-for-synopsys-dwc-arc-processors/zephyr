# Copyright (c) 2025 Synopsys Inc.
#
# SPDX-License-Identifier: Apache-2.0

'''lldbac runner using run-lldbac tool.

This runner provides `flash`/`debug`/`debugserver` for hardware (default) and simulator.

Command Overview:
  flash       - Load and run code on hardware (exits immediately)
  debug       - Interactive debugging session (loads code, provides debugger UI)
  debugserver - Start GDB server only (client loads code when connecting)

Usage Examples:
  Hardware mode (default):
    west flash --runner lldbac --board-json=myboard.json
    west debug --runner lldbac --board-json=myboard.json
    west debugserver --runner lldbac --board-json=myboard.json

  Simulator mode:
    west debug --runner lldbac --simulator
    west debugserver --runner lldbac --simulator

Why Three Commands?
  - flash: Quick execution without debugging overhead
  - debug: Interactive debugging with immediate code loading
  - debugserver: Remote debugging - server doesn't load code, connecting client does
    This matches standard debugserver behavior (OpenOCD, JLink, etc.)

Note on simulator flash:
  Flash is intentionally not supported for simulator mode. Use the arc-nsim
  runner for simulator execution without debugging: 'west flash --runner arc-nsim'
  The lldbac runner is specifically designed for interactive debugging workflows.

Configuration:
  --simulator: Use simulator instead of physical hardware (default is hardware)
  --tcf: Tool Configuration File for target configuration (optional)
         For simulator: mutually exclusive with --nsim-props
         For hardware: can be used alongside --board-json
  --nsim-props: nsim properties filename (board-specific, no default)
                Only used for simulator mode when --tcf is not specified
  --board-json: board.json file for hardware connection (required for hardware mode)
  --gdb-port: Port number for debugserver command (default: 3333)

Debug Server Mode:
  The debugserver command starts lldbac in GDB server mode, allowing you to
  connect a separate debugger client from another terminal. The server does NOT
  load code - the connecting client is responsible for loading.

  Hardware debugging:
    Start server (Terminal 1):
      west debugserver --runner lldbac --board-json=board.json

    Connect client (Terminal 2):
      lldbac -o 'gdb-remote localhost:3333' -o load build/zephyr/zephyr.elf

  Simulator debugging:
    west debugserver --runner lldbac --simulator

    Connect client (Terminal 2):
      lldbac -o 'gdb-remote localhost:3333' -o load build/zephyr/zephyr.elf

  Alternative: Using lldbac client with arc-nsim runner (simulator only):
    For simulator debugging, you can also leverage the arc-nsim runner's native
    nSIM debugserver support and connect with lldbac as the client:

    Start server (Terminal 1):
      west debugserver --runner arc-nsim

    Connect client (Terminal 2):
      lldbac -o 'gdb-remote localhost:3333' -o load build/zephyr/zephyr.elf

    This approach uses nSIM's native GDB server (nsimdrv -gdb) with lldbac
    as the debugging client, providing maximum compatibility with nSIM features.

  This matches the behavior of OpenOCD, JLink, and nsim debugserver commands.

Hardware Mode (default):
  Hardware mode requires a board.json file containing JTAG configuration.
  The file path must be explicitly specified with --board-json argument.

  Expected board.json format:
  [
    {
      "connect": "jtag-digilent",
      "jtag_device": "JtagHs2",
      "jtag_frequency": "500KHz",
      "postconnect": ["command source preload.cmd"]
    }
  ]

  Note: The postconnect field is optional. You can add any run-lldbac
  compatible parameters to the board.json configuration.

Board Configuration:
  Each board should specify its nsim properties file in board.cmake:
    board_runner_args(lldbac "--nsim-props=board_specific.props")

  For hardware support, users must explicitly specify the board.json file:
    west debug --runner lldbac --board-json=path/to/board.json

  For simulator support with nsim, specify the props file in board.cmake:
    west debug --runner lldbac --simulator
'''

import shutil
import sys
from os import path

from runners.core import RunnerCaps, ZephyrBinaryRunner


class LldbacBinaryRunner(ZephyrBinaryRunner):
    '''Runner front-end for run-lldbac tool for simulator and hardware.'''

    def __init__(self, cfg, props, tcf=None, simulator=False, board_json=None, gui=False, gdb_port=None):
        super().__init__(cfg)
        # Use shutil.which() to find run-lldbac, which automatically handles
        # platform-specific extensions (.bat, .exe on Windows)
        lldbac_exe = shutil.which('run-lldbac')
        if lldbac_exe is None:
            # Fallback to plain name if not found in PATH
            lldbac_exe = 'run-lldbac'
        self.lldbac_cmd = [lldbac_exe]
        self.tcf = tcf
        self.props = props
        self.simulator = simulator
        self.board_json = board_json
        self.gui = gui
        self.gdb_port = gdb_port

    @classmethod
    def name(cls):
        return 'lldbac'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            '--simulator', action='store_true', help='Use simulator instead of physical hardware (default is hardware)'
        )
        parser.add_argument('--tcf', help='TCF (Tool Configuration File) for target configuration')
        parser.add_argument(
            '--nsim-props', help='nsim props filename (looked up under board support)'
        )
        parser.add_argument(
            '--board-json',
            help='board.json file for hardware connection (required for hardware mode)',
        )
        parser.add_argument('--gui', action='store_true', help='Launch VS Code GUI for debugging')
        parser.add_argument('--gdb-port', type=int, default=3333,
                          help='GDB server port for debugserver command (default: 3333)')

    @classmethod
    def do_create(cls, cfg, args):
        chosen_props = args.nsim_props

        # Get board.json for hardware mode (default mode)
        board_json = None
        if not args.simulator:
            board_json = cls._get_board_json(cfg, args)

        return LldbacBinaryRunner(
            cfg,
            props=chosen_props,
            tcf=args.tcf,
            simulator=args.simulator,
            board_json=board_json,
            gui=args.gui,
            gdb_port=args.gdb_port,
        )

    @classmethod
    def _get_board_json(cls, cfg, args):
        '''Get board.json file path for hardware mode'''
        if not args.board_json:
            raise ValueError(
                "Hardware mode requires --board-json argument. "
                "Specify the board.json file path with --board-json"
            )

        # Handle relative or absolute path
        if path.isabs(args.board_json):
            board_json_path = args.board_json
        else:
            board_json_path = path.join(cfg.board_dir, args.board_json)

        # Verify the file exists
        if not path.exists(board_json_path):
            raise ValueError(f"Board JSON file not found: {board_json_path}")

        return board_json_path

    def do_run(self, command, **kwargs):
        # Log mode selection to help diagnose environment issues
        config = f"tcf={self.tcf}" if self.tcf else f"props={self.props}"
        mode = 'sim' if self.simulator else 'hardware'
        self.logger.info(f"lldbac: mode={mode} {config}")
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
        '''Flash and run on physical hardware using run-lldbac.

        Note: Flash is intentionally not supported for simulator mode.
        The arc-nsim runner already provides efficient simulator execution
        without debugging overhead. Use 'west flash --runner arc-nsim' for
        simulator runs.
        '''
        if self.simulator:
            raise ValueError(
                "Flash command is only available for hardware (default mode). "
                "For simulator execution, use arc-nsim runner: "
                "'west flash --runner arc-nsim'"
            )

        self.require(self.lldbac_cmd[0])

        # Create run-lldbac script for hardware flash using --cores-json
        lldbac_cmds = [
            self.lldbac_cmd[0],
            '--batch',  # Exit after executing commands
            '--cores-json',
            self.board_json,
            '-o',
            'run',
        ]

        if self.tcf:
            lldbac_cmds.extend(['--tcf', self.tcf])

        if self.gui:
            lldbac_cmds.append('--gui')

        lldbac_cmds.append(self.cfg.elf_file)

        self.logger.info(f"Flashing to hardware using board.json: {self.board_json}")
        self.check_call(lldbac_cmds)

    def do_debug(self, **kwargs):
        '''Start interactive debugging session'''
        if self.simulator:
            self._do_debug_simulator(**kwargs)
        else:
            self._do_debug_hardware(**kwargs)

    def _do_debug_simulator(self, **kwargs):
        '''Debug on nsim simulator'''
        # run-lldbac with --nsim manages nSIM internally
        lldbac_cmd = self.lldbac_cmd.copy()

        if self.tcf:
            # TCF mode: use TCF file for target configuration
            lldbac_cmd.extend(['--tcf', self.tcf])
        else:
            # Props mode: use nsim properties for target configuration
            nsim_props_file = self._get_nsim_props()
            self.logger.info(f"lldbac: using nsim props: {nsim_props_file}")
            nsim_props_string = self._read_nsim_props(nsim_props_file)
            lldbac_cmd.extend(['--nsim', nsim_props_string])

        lldbac_cmd.extend(['-o', 'load'])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        # Run run-lldbac interactively
        self.run_client(lldbac_cmd)

    def _do_debug_hardware(self, **kwargs):
        '''Debug on physical hardware'''
        self.require(self.lldbac_cmd[0])

        # Interactive debug session on hardware using --cores-json
        lldbac_cmd = self.lldbac_cmd + ['--cores-json', self.board_json]

        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])

        if self.gui:
            lldbac_cmd.append('--gui')

        lldbac_cmd.append(self.cfg.elf_file)

        self.logger.info(f"Starting debug session on hardware using board.json: {self.board_json}")
        # Run interactively
        self.run_client(lldbac_cmd)

    def _get_nsim_props(self):
        '''Get nSIM properties file path: board_dir/support/<props>.'''
        if not self.props:
            raise ValueError(
                "nsim-props filename is required but not provided. "
                "Either configure it in board.cmake or use --tcf for TCF-based configuration"
            )

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

    def _read_nsim_props(self, props_file):
        '''Read nSIM properties file and convert to string format for run-lldbac.

        Reads a props file with lines like:
            nsim_isa_family=rv32
            nsim_isa_ext=All

        And converts to a space-separated string:
            "nsim_isa_family=rv32 nsim_isa_ext=All"
        '''
        properties = []
        with open(props_file, encoding='utf-8') as f:
            for line in f:
                # Strip whitespace and comments
                line = line.strip()
                if line and not line.startswith('#'):
                    properties.append(line)

        return ' '.join(properties)

    def do_debugserver(self, **kwargs):
        '''Start a debug server for remote GDB connection.

        This starts lldbac in server mode, allowing a separate GDB client
        to connect on the specified port. This is cleaner than sharing
        the console in non-GUI mode.
        '''
        if self.simulator:
            self._do_debugserver_simulator(**kwargs)
        else:
            self._do_debugserver_hardware(**kwargs)

    def _do_debugserver_simulator(self, **kwargs):
        '''Start debug server on nsim simulator.

        Note: debugserver does NOT load code - the connecting client is responsible
        for loading. This matches the behavior of other debugserver implementations
        (OpenOCD, JLink, nsim) and keeps the server lightweight.
        '''
        lldbac_cmd = self.lldbac_cmd.copy()

        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])
        else:
            nsim_props_file = self._get_nsim_props()
            self.logger.info(f"lldbac: using nsim props: {nsim_props_file}")
            nsim_props_string = self._read_nsim_props(nsim_props_file)
            lldbac_cmd.extend(['--nsim', nsim_props_string])

        # Start in server mode - client will load code when connecting
        lldbac_cmd.extend([
            '-o', 'gdb-remote listen:*:{}'.format(self.gdb_port)
        ])

        # Do NOT pass ELF file - client loads it when connecting

        self.logger.info(f"Starting debug server on port {self.gdb_port}")
        self.logger.info(f"Connect with: lldbac -o 'gdb-remote localhost:{self.gdb_port}' -o load {self.cfg.elf_file}")
        self.check_call(lldbac_cmd)

    def _do_debugserver_hardware(self, **kwargs):
        '''Start debug server on physical hardware.

        Note: debugserver does NOT load code - the connecting client is responsible
        for loading. This matches the behavior of other debugserver implementations
        (OpenOCD, JLink, nsim) and keeps the server lightweight.
        '''
        lldbac_cmd = self.lldbac_cmd + ['--cores-json', self.board_json]

        if self.tcf:
            lldbac_cmd.extend(['--tcf', self.tcf])

        # Start in server mode - client will load code when connecting
        lldbac_cmd.extend([
            '-o', 'gdb-remote listen:*:{}'.format(self.gdb_port)
        ])

        # Do NOT pass ELF file - client loads it when connecting

        self.logger.info(f"Starting debug server on port {self.gdb_port} using board.json: {self.board_json}")
        self.logger.info(f"Connect with: lldbac -o 'gdb-remote localhost:{self.gdb_port}' -o load {self.cfg.elf_file}")
        self.check_call(lldbac_cmd)
