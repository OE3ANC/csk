简介
=====================
演示如何使用 Zephyr 的按键扫描 (kscan) API 与 FT5336 触控控制器进行交互。运行该代码后，按键扫描事件将被打印到控制台。

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/kscan/ft5336/ -p

功能说明
=====================
1. 从设备树中定义的 ft5336 节点获取 kscan 设备的配置信息

::

    const struct device *kscan_dev = DEVICE_DT_GET(DT_NODELABEL(ft5336));

2. 检查 kscan 设备是否就绪

::

    if (!device_is_ready(kscan_dev)) {
        printk("kscan device not ready \n");
        return -1;
    }

3. 配置 kscan 设备的回调函数，并启用回调

::

    ret = kscan_config(kscan_dev, kb_callback);
    kscan_enable_callback(kscan_dev);

4. 回调函数打印按键事件的行列号和按键状态

::

    static void kb_callback(const struct device *dev, uint32_t row, uint32_t col, bool pressed)
    {
        ARG_UNUSED(dev);

        printk("Key row = %d col = %d Pressed = %u\n", row, col, pressed);
    }

运行
======================
按下开发板复位按钮，运行程序。在控制台中可以看到按键扫描事件的打印信息。





