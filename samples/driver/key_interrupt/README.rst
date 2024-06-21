简介
========================
演示如何使用 MCU 的按键中断功能，通过按键 K3 按键中断控制输出串口信息。

构建步骤
========================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/key_interrupt/ -p


功能说明
=======================
1.从设备树中获取按键设备的配置信息

::

    static const struct gpio_dt_spec key_3 = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

2.定义回调按键中断回调函数

::

    static struct gpio_callback key_3_cb_data;

3.设置信号量

::

    K_SEM_DEFINE(key_sem, 0, 1);

4.按键初始化，设置并添加回调

::

    r = key_init();

5.串口打印，判断按键中断是否成功

::

    key_interrupt_led();

运行
========================
按下开发板复位按钮，运行程序，按下按键，可观察串口信息。




