简介
===================
使用扩展MCU的io来闪烁LED。展示了如何将GPIO引脚配置为输出，然后打开和关闭它们，控制开发板上的的三色灯进行闪烁。

构建步骤
==================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/exmcu_gpio_led/ -p

功能说明
==================
1.验证 GPIO 端口是否已准备就绪

::	

    gpio_is_ready_dt()

2.基于设备树节点的配置来设置特定的 GPIO 引脚

::

    gpio_pin_configure_dt()

3.循环中切换 GPIO 引脚的状态

::
    
    gpio_pin_toggle_dt()

运行
=======================
按下开发板复位按钮，运行程序,观察LED灯的闪烁变化