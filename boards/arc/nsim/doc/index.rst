.. _nsim:

DesignWare(R) ARC(R) Emulation (nsim)
#####################################

Overview
********

This board can be used to run ARC EM / ARC HS based images in simulation with `Designware ARC nSIM`_
or run same images on FPGA based HW platform `HAPS`_. The board includes the following features:

* ARC processor (can be either ARCv2 or ARCv3 ISA; either ARC HS or ARC EM type)
* ARC internal timer
* a virtual serial console (ns16550 based UART model)

In addition to that some configurations include other components like:

* various kinds of closed coupled memories (CCM) - ICCM, DCCM, XCCM, YCCM, ...
* ARConnect functionality (usually enabled in multi-core configurations) - inter core interrupt
  unit (ICI), interrupt distribution unit (IDU), global free running counter (GFRC), intercore debug
  unit (ICD), ...

You can find more information about ARC features support in Zephyr
:ref:`here <hardware_arch_arc_support_status>`

There are multiple supported board sub-configurations. For example, some (but not all) of them:

* ``nsim_em`` - normal ARCv2 EM features and ARC MPUv2
* ``nsim_em_em7d_v22`` - normal ARCv2 EM features and ARC MPUv2, with one register bank and fast irq
* ``nsim_em_em11d`` - normal ARCv2 EM features and ARC MPUv2, with XY memory and DSP feature
* ``nsim_sem`` - ARCv2 secure EM (SEM) features and ARC MPUv4
* ``nsim_hs`` - ARCv2 HS features, i.e. w/o PMU and MMU
* ``nsim_hs_smp`` - ARCv2 HS features in multi-core cluster, still w/o PMU and MMU
* ``nsim_hs5x`` - base ARCv3 ARC HS 32-bit features, i.e. w/o PMU and MMU
* ``nsim_hs6x`` - base ARCv3 ARC HS 64-bit features, i.e. w/o PMU and MMU

.. _board_arc_nsim_prop_args_files:

For detailed arc features of each board sub-configuration, please refer to configuration files in
:zephyr_file:`boards/arc/nsim/support/` directory.

In case of single-core configurations it would be ``.props`` file (which contains configuration
for nSIM simulator) and ``.args`` file (which contains configuration for MDB debugger).
Note that these file contains identical HW configuration and we use only one of them at the same
time - ``.props`` file if we do run on standalone nSIM simulator and ``.args`` file if we do run
nSIM via MDB or do debug on nSIM launched via MDB.

.. hint::
   If you see different tests behavior when you run and debug test application (especially if you've
   created new board or modified existing one) - check that features defined in ``.props`` and
   ``.args`` are identical.

I.e. for the single-core ``nsim_hs5x`` board we have
:zephyr_file:`boards/arc/nsim/support/nsim_hs5x.props` and
:zephyr_file:`boards/arc/nsim/support/mdb_hs5x.args`

For the multi-core configurations we have only ``.args`` file as the milti-core configuration
can only be run / debug on nSIM launched via MDB debugger.

I.e. for the milti-core ``nsim_hs5x_smp`` board we have only
:zephyr_file:`boards/arc/nsim/support/mdb_hs5x_smp.args`

.. warning::
   All these configurations are used for demo and testing purposes. They are not meant to be
   'fixed' and they may be renamed, removed or modified without a notice.

Programming and Debugging
*************************

Required Hardware and Software
==============================

To run single-core Zephyr RTOS applications in simulation on this board,
`Designware ARC nSIM`_ or `Designware ARC nSIM Lite`_ is required.

To run multi-core Zephyr RTOS applications in simulation on this board,
`Designware ARC nSIM`_ and MetaWare Debugger from `ARC MWDT`_ are required.

To run Zephyr RTOS applications on FPGA-based `HAPS`_ platform,
MetaWare Debugger from `ARC MWDT`_ is required as well as the HAPS platform itself.

Building & Running Sample Applications
======================================

Most board sub-configurations support building with both GNU and ARC MWDT toolchains, however
there might be exceptions from that, especially for new added targets. You can check supported
toolchains for the sub-configurations in the correspondent ``.yaml`` file.

I.e. for the ``nsim_hs5x`` board we can check :zephyr_file:`boards/arc/nsim/nsim_hs5x.yaml`

The supported toolchains are listed in ``toolchain:`` array in ``.yaml`` file, where we can find:

* **zephyr** - implies ARC GNU toolchain from Zephyr SDK. You can find bore information about
  Zephyr SDK :ref:`here <toolchain_zephyr_sdk>`.
* **cross-compile** - implies ARC GNU toolchain not from Zephyr SDK. Note that some (especially new)
  sub-configurations may declare ``cross-compile`` toolchain support without ``zephyr`` toolchain
  support because correspondent target CPU support hasn't been added to Zephyr SDK yet.
  You can find more information about its usage :ref:`here <other_x_compilers>`.
* **arcmwdt** - implies proprietary ARC MWDT toolchain. You can find more information about its usage
  :ref:`here <toolchain_designware_arc_mwdt>`.

.. note::
   Note that even if both GNU and MWDT toolchain support is declared for the target some tests or
   samples can be only build with either GNU or MWDT toolchain due to some features limited to exact
   toolchain.

Use this configuration to run basic Zephyr applications and kernel tests in
nSIM, for example, with the :ref:`synchronization_sample`:

.. zephyr-app-commands::
   :zephyr-app: samples/synchronization
   :host-os: unix
   :board: nsim_em
   :goals: flash

This will build an image with the synchronization sample app, boot it using
nsim, and display the following console output:

.. code-block:: console

      *** Booting Zephyr OS build zephyr-v3.2.0-3948-gd351a024dc87 ***
      thread_a: Hello World from cpu 0 on nsim!
      thread_b: Hello World from cpu 0 on nsim!
      thread_a: Hello World from cpu 0 on nsim!
      thread_b: Hello World from cpu 0 on nsim!
      thread_a: Hello World from cpu 0 on nsim!


.. note::
   To exit the simulator, use Ctrl+], then Ctrl+c

.. _board_arc_nsim_verbose_build:

.. tip::
   You can get more details about building process by running build in verbose mode. It can be done
   by passing ``-v`` flag to the west: ``west -v build -b nsim_hs samples/synchronization``

You can run applications build for nsim board not only on nSIM simulation itself, but also on FPGA
based HW platform `HAPS`_. To run previously built application on HAPS do:

.. code-block:: console

   west flash --runner mdb-hw

.. note::
   To run on HAPS, in addition to proper build and flash Zephyr image, you need setup HAPS itself
   as well as flash proper built FPGA image (aka bit-file). This instruction doesn't cover those
   steps, so you need to follow HAPS manual.

Debugging
=========

.. _board_arc_nsim_debugging_mwdt:

Debugging with MDB
------------------

.. note::
   We strongly recommend to debug with MetaWare debugger (MDB) as:

   * it supports wider amount of ARC-specific features
   * it allows to debug both single-core and multi-core nsim targets.
   * it allows to debug on `HAPS`_ platform.

You can use following command to start GUI debugging when running application on nSIM simulator
(no mater if we are using single-core or multi-core configuration):

.. code-block:: console

   west debug --runner mdb-nsim

You can use following command to start GUI debugging when running application on `HAPS`_ platform:

.. code-block:: console

   west debug --runner mdb-hw

.. tip::
   The ``west debug`` (as well as ``west flash``) is just a wrapper script and so it's possible to
   extract the exact commands which are called in it by running it in verbose mode. For that you
   need to pass ``-v`` flag to the wrapper. For example, if you run the following command:

   .. code-block:: console

      west -v debug --runner mdb-nsim

   It will produce the following output (the ``nsim_hs5x_smp`` configuration was used for that
   example):

   .. code-block:: console

       < *snip* >
      -- west debug: using runner mdb-nsim
      runners.mdb-nsim: mdb -pset=1 -psetname=core0 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      runners.mdb-nsim: mdb -pset=2 -psetname=core1 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      runners.mdb-nsim: mdb -multifiles=core1,core0 -OKN

   From that output it's possible to extract MDB commands used for setting-up the GUI debugging
   session:

   .. code-block:: console

      mdb -pset=1 -psetname=core0 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      mdb -pset=2 -psetname=core1 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -nsim @/path/zephyr/boards/arc/nsim/support/mdb_hs5x_smp.args /path/zephyr/build/zephyr/zephyr.elf
      mdb -multifiles=core1,core0 -OKN

   Then it's possible to use them directly or in some machinery if required.

   .. warning::
      It is strongly recommended to not rely on mdb command line options listed above but
      extract it yourself for your configuration.

   .. note::
      If you are planning to run or debug application with MDB on multi-core configuration on nSIM
      simulator without ``west flash`` and ``west debug`` wrappers please don't forget to
      set :envvar:`NSIM_MULTICORE` environment variable to ``1``. If you are using ``west flash`` or
      ``west debug`` it's done automatically by wrappers.

Debugging with GDB
------------------

.. note::
   Debugging on nSIM via GDB is only supported on single-core configurations (which use standalone
   nSIM). However if you are able to launch application on multi-core nsim target that means you can
   simply :ref:`debug with MDB debugger <board_arc_nsim_debugging_mwdt>`.
   It's the nSIM with ARC GDB restriction, real HW multi-core ARC targets can be debugged with ARC
   GDB.

.. note::
   Currently we don't support debug with GDB on `HAPS`_ platform.

.. note::
   The normal ``west debug`` command won't work for debugging applications using nsim boards
   because both the nSIM simulator and the debugger (either GDB or MDB) use the same console for
   input / output.
   In case of GDB debugger you can use separate terminal windows for GDB and nSIM to avoid
   intermixing their output. For the MDB debugger you can simply use GUI bode.

After building your application, open two terminal windows. In terminal one, use nSIM to start a GDB
server and wait for a remote connection with following command:

.. code-block:: console

   west debugserver --runner arc-nsim

In terminal two, connect to the GDB server using ARC GDB. You can find it in Zephyr SDK:

* for the ARCv2 targets you should use :file:`arc-zephyr-elf-gdb`
* for the ARCv3 targets you should use :file:`arc64-zephyr-elf-gdb`

This command loads the symbol table from the elf binary file, for example the
:file:`build/zephyr/zephyr.elf` file:

.. code-block:: console

   arc-zephyr-elf-gdb  -ex 'target remote localhost:3333' -ex load build/zephyr/zephyr.elf

Now the debug environment has been set up, you can debug the application with gdb commands.

Modifying the configuration
***************************

If you've decided to modify existing nsim configuration or even create a new one it's required
to maintain alignment between

* Zephyr OS configuration
* nSIM & MDB configuration
* GNU & MWDT toolchain compiler options

.. note::
   The ``.tcf`` configuration files are not supported by Zephyr directly. There are multiple
   reasons for that. ``.tcf`` perfectly suits when we building bare-metal single-thread application -
   in that case we want to pass all the compiler options from ``.tcf`` to compiler, so all the HW
   features will be used by application and we get optimal code.
   Ihe simulation is completely different when we are talking about building the multi-thread
   features-reach operation system. Of course we still can simply build all code with all the
   options from ``.tcf`` - but that may be far from optimal solution. For example, such approach
   require so save & restore full register context for all tasks (and sometimes even for
   interrupts). And for DSP-enabled or for FPU-enabled systems that leads to dozens of extra
   registers save and restore even if the most of the user and kernel tasks don't actually use
   DSP or FPU. Instead we prefer to fine-tune the HW features usage which (with all its pros)
   require us to maintain the separate from ``.tcf`` configuration.


Zephyr OS configuration
=======================

Zephyr OS configuration is defined via Kconfig and Device tree. These are non ARC-specific
mechanisms which are described in :ref:`board porting guide <board_porting_guide>`.

Generally you can look for ``<board_name>_defconfig``, ``<board_name>.dts`` and
``<board_name>.yaml`` as an entry point for board configuration.

nSIM configuration
==================

nSIM configuration is defined in :ref:`props and args files <board_arc_nsim_prop_args_files>`.
Generally they are identical to the values from correspondent ``.tcf`` configuration with a few
exceptions:

* The UART model is added (to both ``.props`` and ``.args`` files).
* Options to fine-tune MDB behavior are added (to ``.args`` files only) to disable MDB profiling and
  fine-tune MDB behavior on multi-core systems.

GNU & MWDT toolchain compiler options
=====================================

The hardware-specific compiler options are set in correspondent SoC cmake file. For ``nsim`` board
it is :zephyr_file:`soc/arc/snps_nsim/CMakeLists.txt`.

For the GNU toolchain the basic configuration is set via mcpu which is defined in generic code
and based on the selected CPU model via Kconfig. It still can be forcedly set to required value
on SoC level.

For the MWDT toolchain we set all hardware-specific compiler options directly in SoC
``CMakeLists.txt``.

.. note::
   The non hardware-specific compiler options like optimizations, library selections, C / C++
   language options are still set in Zephyr generic code. You can verify them by
   :ref:`running build in verbose mode <board_arc_nsim_verbose_build>`.

References
**********

.. _Designware ARC nSIM: https://www.synopsys.com/dw/ipdir.php?ds=sim_nsim
.. _Designware ARC nSIM Lite: https://www.synopsys.com/cgi-bin/dwarcnsim/req1.cgi
.. _HAPS: https://www.synopsys.com/verification/prototyping/haps.html
.. _ARC MWDT: https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware
