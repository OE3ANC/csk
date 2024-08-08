简介
=====================
Tip:如果使用的是csk6_duomotai开发板,需要短接开发板上的SD_CARD引脚

当PB05引脚检测到电平变化(检测电平0.4V)时,会触发中断进行ADC采集PB05上的电压,当电压值满足“<1100 1890 3300>”这三个数值(位于csk6_duomotai_devkit.overlay文件),就会将采集信息打印到控制台

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/keysense/ -p

功能说明
=====================
1. 从设备树中定义的 keysense 节点获取 kscan 设备的配置信息

::

    const struct device *kscan_dev = DEVICE_DT_GET(DT_NODELABEL(keysense));

2. 检查 kscan 设备是否就绪

::

	if (!device_is_ready(kscan_dev)) {
		printk("kscan device  not ready \n");
		return -1;
	}

3. 配置 kscan 设备的回调函数

::

	ret = kscan_config(kscan_dev, kb_callback);

4. 回调函数打印按键事件的行列号和按键状态

::

    static void kb_callback(const struct device *dev, uint32_t row, uint32_t col, bool pressed)
    {
        ARG_UNUSED(dev);

        printk("Key row = %d col = %d Pressed = %u\n", row, col, pressed);
    }

运行
======================
按下开发板复位按钮，运行程序。给定 PB05 下降沿跳变输入电压(如3.3V到0V)，在控制台中可以看到按键扫描事件的打印信息。

