简介
=====================
演示如何使用 UART 驱动程序配置和更改 UART 波特率，并且每隔一段时间打印 "hello world"。

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/uart/uart_configure/ -p

功能说明
=====================
1.从设备树中获取设备的句柄

::

    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

2.获取当前的 UART 的配置

::

    uart_config_get(dev, &cfg) != 0

3.打印配置

::

    print_uart_config(&cfg);

4.更改 UART 配置

::

    uart_configure(dev, &cfg) != 0

5.进入一个无限循环，每 500 毫秒打印一次 "hello world"。

::

    	while (1) {
		printk("hello world\n");
		k_msleep(500);
	}

运行
======================
按下开发板复位按钮，可通过串口查看输出结果

