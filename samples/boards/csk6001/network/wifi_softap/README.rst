.. _wifi_softap:

CSK6001 WiFi SoftAP
###################

Overview
********

This sample demonstrates how to use csk6001 to startup a WiFi hotspot

Building and Running
********************

Make sure you have the CSK6001 connected over USB port.

The sample can be built and flashed as follows:

.. code-block:: console

   west build -b csk6001 samples/boards/csk6001/wifi_softap
   west flash

Sample Output
=============

To check output of this sample, any serial console program can be used (i.e. on Linux minicom, putty, screen, etc)
This example uses ``minicom`` on the serial port ``/dev/ttyUSB0``:

.. code-block:: console

   $ minicom -D /dev/ttyUSB0 -b 115200

.. code-block:: console

    *** Booting Zephyr OS build f4ee5f98ad0b  ***
   ssid: CSK6001_SoftAP password: 12345678
   [WiFi softap] started
   [WiFi softap] station joined
