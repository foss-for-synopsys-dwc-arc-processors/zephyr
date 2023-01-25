## HSDK-4xD support

This branch adds HSDK-4xD board support. The HSDK-4xD support was made on top of `hsdk` Zephyr board effectively superceeding the original HSDK in that particular source tree (in the upstream project both the boards will co-exist), hence to build for HSDK-4xD target from this source tree one needs to use `hsdk` Zephyr platform.

Both GNU toolchain (from Zephyr SDK) and ARC MWDT toolchain are supported. The support was verified with Zephyr testsuite which was run on `hsdk` Zephyr board with both the toolchains. Please note that few test were disabled as there're known issues associated with them.

## Building a sample application for Zephyr RTOS

### Building with GNU toolchain (from the Zephyr SDK) on Linux host

It's expected that Zephyr SDK (https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.15.0) was installed as well as all other needed environment configuration steps from generic Zephyr getting started guide were done, see https://docs.zephyrproject.org/latest/develop/getting_started/index.html.

```
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=<_your_sdk_instalation_path_>/zephyr-sdk-0.15.0

west build -b hsdk samples/hello_world
```

**NOTE:** That Zephyr RTOS snapshot and HSDK-4xD support in it was tested with Zephyr SDK v0.15.0.

### Building with ARC MWDT toolchain on Linux host

Proprieraty ARC MWDT toolchain is not included in the Zephyr SDK and so needs to be installed separately, please refer to MetaWare Development Toolkit documentation here https://www.synopsys.com/dw/ipdir.php?ds=sw_metaware. Even though ARC MWDT compiler will be used for Zephyr RTOS sources compilation, still the GNU preprocessor & GNU objcopy might be used for some steps like DTS preprocessing and `.bin` file generation.

It's expected that Zephyr SDK (https://github.com/zephyrproject-rtos/sdk-ng/releases/tag/v0.15.0) was installed as well as all other needed environment configuration steps from generic Zephyr getting started guide were done, see https://docs.zephyrproject.org/latest/develop/getting_started/index.html.

```
export ZEPHYR_TOOLCHAIN_VARIANT=arcmwdt
export ZEPHYR_SDK_INSTALL_DIR=<_your_sdk_instalation_path_>/zephyr-sdk-0.15.0
export ARCMWDT_TOOLCHAIN_PATH=$METAWARE_ROOT/..

west build -b hsdk samples/hello_world
```

**NOTE:** That Zephyr RTOS snapshot and HSDK-4xD support in it was tested with MWDT v2022.12 and the Zephyr SDK v0.15.0.

## Running & debugging Zephyr RTOS-based applications on HSDK-4xD board

The board uses UART for printing / logging / console which is accessible from a development host via on-board UART-USB adapter. To access it one can use `minicom` (https://linux.die.net/man/1/minicom) or any other terminal emulator tool.

```
minicom -D /dev/ttyUSB0 -b 115200 -c on
```

**NOTE:** The device name is generated dynamically by the host OS, so it may differ from `/dev/ttyUSB0` (typically still `/dev/ttyUSBx`, where "x" is a small numeric value).

### Running with GNU tools

After the building step (with either GNU or MWDT toolchains) just use the following command to run the built application on HSDK-4xD board.

**NOTE:** It is strongly recommended to reset the board before each execution of an application! Otherwise previously introduced internal states of ARC cores and/or peripheral devices might produce unexpected behavior on the new execution.

```
west flash
```

**NOTE:** In case multiple boards are connected to the same host it will be necessary to provide the board's serial number for proper selection of the device to be used, i.e:

```
west flash --serial=251642000185
```

### Running with MWDT tools

After building step (with either GNU or MWDT toolchains) following command needs to be used to run the built application on HSDK-4xD board.

**NOTE:** It is strongly recommended to reset the board before each execution of an application! Otherwise previously introduced internal states of ARC cores and/or peripheral devices might produce unexpected behavior on the new execution.

```
west flash --runner mdb-hw
```

**NOTE:** In case multiple boards are connected to the same host it will be necessary to provide the board's Digilent "User Name" for proper selection of the device to be used, i.e:

```
west flash --runner mdb-hw --dig-device=HSDK-4xD
```

**HINT:** `west flash` is just a wrapper script and so it's possible to extract the exact commands which are called by it by passing `-v` flag to the wrapper.
For example, running the following command:

```
west -v flash --runner mdb-hw
```

It will produce the following output:
```
 < *snip* >
-- west flash: using runner mdb-hw
runners.mdb-hw: mdb -pset=1 -psetname=core0 '' -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
runners.mdb-hw: mdb -pset=2 -psetname=core1 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
runners.mdb-hw: mdb -pset=3 -psetname=core2 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
runners.mdb-hw: mdb -pset=4 -psetname=core3 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
runners.mdb-hw: mdb -multifiles=core3,core2,core1,core0 -run '-cmd=-nowaitq run' -cmd=quit -cl
```

From that output it's possible to extract MDB commands used for setting-up the execution session:

```
mdb -pset=1 -psetname=core0 '' -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
mdb -pset=2 -psetname=core1 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
mdb -pset=3 -psetname=core2 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
mdb -pset=4 -psetname=core3 -prop=download=2 -nooptions -nogoifmain -toggle=include_local_symbols=1 -digilent /path/zephyrproject/zephyr/build/zephyr/zephyr.elf
mdb -multifiles=core3,core2,core1,core0 -run '-cmd=-nowaitq run' -cmd=quit -cl
```

Then it's possible to use them directly (if required) or remove `-run '-cmd=-nowaitq run' -cmd=quit -cl` part and start GUI debugging session.
