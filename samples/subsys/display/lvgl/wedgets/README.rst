.. _lvgl-sample:

LittlevGL Basic Sample
######################

Overview
********

This sample application displays "Hello World" in the center of the screen
and a counter at the bottom which increments every second. If an input driver
is supported, such as the touch panel controller on mimxrt10{50,60,64}_evk
boards, "Hello World" is enclosed in a button that changes to the toggled state
when touched.

Building and Running
********************

Make sure you have the CSK6 connected over USB port.

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b csk6 samples/subsys/csk_malloc
   west flash

Requirements
************

Building and Running
********************

.. note::
   When deferred logging is enabled you will likely need to increase
   :kconfig:option:`CONFIG_LOG_STRDUP_BUF_COUNT` and/or
   :kconfig:option:`CONFIG_LOG_STRDUP_MAX_STRING` to make sure no messages are lost or
   truncated.

Example building for :ref:`nrf52840dk_nrf52840`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl/wedgets
   :board: nrf52840dk_nrf52840
   :shield: adafruit_2_8_tft_touch_v2
   :goals: build flash

Example building for :ref:`native_posix`:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/lvgl/wedgets
   :board: native_posix
   :goals: build run

Alternatively, if building from a 64-bit host machine, the previous target
board argument may also be replaced by ``native_posix_64``.

References
**********

.. target-notes::

.. _LittlevGL Web Page: https://littlevgl.com/
.. _SDL2: https://www.libsdl.org
.. _RK043FN02H-CT: https://www.nxp.com/products/processors-and-microcontrollers/arm-based-processors-and-mcus/i.mx-applications-processors/i.mx-rt-series/4.3-lcd-panel:RK043FN02H-CT
