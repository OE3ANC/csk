.. _wifi_softap:

CSK6 WiFi SoftAP+Station
###################

Overview
********

This sample application demonstrates how to configure and connect to the network using CSK6 SoftAP+Station.

Requirements
************

* CSK6 module (based on Atheros AR9132 Wifi/BT) with SPI interface. It can be purchased from http://www.c-sky.com.tw.

Building and Running
********************

The example will create a soft AP in AP mode and wait for a station to connect. Use the credentials hardcoded in the sample to connect to the AP. The connected clients are announced via console log.

Simply build and flash the application:

.. code-block:: console

   west build -b csk6 samples/network/wifi_softap_sta
   west flash

.. code-block:: console

    *** Booting Zephyr OS build zephyr-v3.4.0-63-g0de75cc2e493 ***
    *** ListenAI CSK SDK Version: 2.0.0.0-7ca5fedab0 ***
    Firmware Version: 0.4, commit: 77c953e
    App EVENT: ESP INIT
    [00:00:02.654,000] <inf> csk_wifi: wifi mode:0
    [00:00:02.675,000] <inf> wifi_app: Wifi Station/softAP Sample
