简介
=====================
演示如何使用 Zephyr 的 GPIO API 控制一个 LED 的闪烁。运行该代码后，开发板上的 LED 将每秒闪烁一次。

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/gpio_led/ -p

功能说明
=====================
1. 从设备树中获取 LED 的配置信息

::

    static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

2. 检查 GPIO 设备是否就绪

::

    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }

3. 配置 GPIO 引脚为输出并设置为初始状态（激活）

::

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);

4. 无限循环中切换 LED 状态并等待 1 秒

::

   ret = gpio_pin_toggle_dt(&led);


运行
======================
按下开发板复位按钮，运行程序。LED 将每秒闪烁一次。



