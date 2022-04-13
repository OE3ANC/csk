.. _scanpen_samples:

CSK6012 9S Scanpen samples
###########

Overview
********

本Sample为scanpen的硬件驱动使用示例合集。
目前已集成：
1. LCD驱动
#. LCD_PWM驱动
#. 触摸屏驱动
#. CAM_LED驱动

注意：因为LCD_PWM需要使用到芯片SDIO PIN配置为PWM功能进行驱动，所以上电5s后将不可下载程序。


Building and Running
********************

The sample can be built and flashed as follows:

.. code-block:: console
   lisa zep build -b csk6012_9s_scanpen
   lisa zep flash

Sample Output
=============

.. code-block:: console

    *** Booting Zephyr OS build c0007e214549  ***
    <inf> sample: 下载模式
    <inf> sample: Display sample for ST7789V
    <inf> sample: PWM模式



