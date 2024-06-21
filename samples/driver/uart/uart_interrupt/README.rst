简介
=========================
演示中断模式下串口1和串口2互相发送数据。

构建步骤
======================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/uart/uart_interrupt/ -p

功能说明
======================
1.从设备树中获取UART0设备

::

	const struct device *uart0 = DEVICE_DT_GET(UART0);

2.从设备树中获取UART1设备

::

	const struct device *uart1 = DEVICE_DT_GET(UART1);

3.定义回调函数

::

	uart_irq_callback_user_data_set(uart0, uart_rx_callback, NULL);

4.使能中断
::

	uart_irq_rx_enable(uart0);

运行
================================
按下开发板复位按钮，运行程序



