.. _embarc_mli_hello_world:

Hello_world Example
###################

Quick Start
--------------

Example supports building with [Zephyr Software Development Kit (SDK)](https://docs.zephyrproject.org/latest/getting_started/installation_linux.html#zephyr-sdk) and running with MetaWare Debuger on [nSim simulator](https://www.synopsys.com/dw/ipdir.php?ds=sim_nSIM).

Add embarc_mli module to Zephyr instruction
-------------------------------------------

1. Open command line and change working directory to './zephyrproject/zephyr'

2. Download embarc_mli version 2.0

        west update

Build with Zephyr SDK toolchain
-------------------------------

    Build requirements:
        - Zephyr SDK toolchain version 0.13.2 or higher
        - gmake

1. Open command line and change working directory to './zephyrproject/zephyr/samples/modules/embarc_mli/hello_world'

2. Build example

        west build -b nsim_em samples/modules/embarc_mli/hello_world

Run example
--------------

1. Run example

        west flash

    Result shall be:

.. code-block:: console

        ...

        in1:

        1 2 3 4 5 6 7 8 

        in2:

        10 20 30 40 50 60 70 80 

        mli_krn_eltwise_add_fx16 output:

        11 22 33 44 55 66 77 88 

        mli_krn_eltwise_sub_fx16 output:

        -9 -18 -27 -36 -45 -54 -63 -72

        ...
