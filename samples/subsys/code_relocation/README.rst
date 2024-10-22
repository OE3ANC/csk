.. _code_relocation:

Code relocation
#################

Overview
********
A simple example that demonstrates how relocation of code, data or bss sections
using a custom linker script.

Building and Running
********************

Make sure you have the CSK6 connected over USB port.

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b csk6 samples/subsys/code_relocation
   west flash

Sample Output
=============

To check output of this sample, any serial console program can be used (i.e. on Linux minicom, putty, screen, etc)
This example uses ``minicom`` on the serial port ``/dev/ttyUSB0``:


.. code-block:: console

   $ minicom -D /dev/ttyUSB0 -b 115200

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.4.0-63-g0de75cc2e493 ***
    *** ListenAI CSK SDK Version: 2.0.0.0-7ca5fedab0 ***
    Address of main function 0x18004969

    Address of function_in_sram2 0x30600001
    Address of var_sram2_data 0x30600200
    Address of k_sem_give 0x30600089
    Address of var_sram2_rodata 0x306001f0
    Address of var_sram2_bss 0x30600204

    Address of function_in_sram 0x800ed
    Hello World! csk6_duomotai_devkit
    Address of custom_section, func placed using attributes 0x30600211
    Address of custom_section data placed using attributes 0x3060020c

    Address of function_in_split_multiple 0x3060008d
    Address of var_file3_sram_data 0x80838
    Address of var_file3_sram2_bss 0x30600208

    out
