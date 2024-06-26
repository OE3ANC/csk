简介
=====================

本文演示了如何在Zephyr RTOS中使用显示驱动和键盘扫描设备。运行该代码后，将在屏幕上显示图形，并通过日志输出响应触摸按键事件。

构建步骤
=====================

lisa zep build -b csk6_duomotai_devkit ./samples/driver/display_kscan/ -p

功能说明
=====================

1. 获取显示设备句柄

::

    display_dev = DEVICE_DT_GET(DT_INST(0, sitronix_st7789v));

2. 获取显示设备能力

::

    display_get_capabilities(display_dev, &capabilities);

3. 初始化键盘扫描设备

::

    kscan_init();

4. 填充缓冲区并写入显示设备

::

    fill_buffer_fnc(TOP_LEFT, 0, buf, buf_size);
    display_write(display_dev, x, y, &buf_desc, buf);

5. 打开显示设备的显示

::

    display_blanking_off(display_dev);

运行
======================
按下开发板复位按钮，运行程序。程序将初始化显示设备和键盘扫描设备，在显示屏上绘制彩色方块，并响应触摸按键事件。
