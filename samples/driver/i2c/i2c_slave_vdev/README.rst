简介
==================
演示如何使用 MCU 的虚拟 i2c 从机通信功能，通过其他 i2c 主设备向从设备的寄存器地址写数据和读数据，使用串口观察结果。

构建步骤
==================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/i2c/i2c_slave_vdev/ -p


功能说明
=================
1.i2c 虚拟从设备初始化

::

    i2_slave_vdev_init();

2.配置i2c 虚拟从设备事件处理回调

::

    slave_vdev_event_handler_register(i2_vdev_0, &vdev_handler);

3.启动i2c 虚拟从设备事件处理任务

::

    start_slave_vdev_handler_task(i2_vdev_0);


运行
====================
按下开发板复位按钮，运行程序，接上其他 i2c 主设备，读写虚拟从设备并观察串口信息。




