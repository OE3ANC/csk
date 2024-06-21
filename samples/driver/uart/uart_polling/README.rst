简介
====================
演示串口自发自收

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/uart/uart_polling/ -p

功能说明
=====================
1.义一个宏 UART0，表示设备树中标记为 uart0 的节点

::

	#define UART0 DT_NODELABEL(uart0)

2.从设备树中获取 UART 设备 uart0

::

	const struct device *uart = DEVICE_DT_GET(UART0);

3.轮询模式下进行 UART 自收自发通信

::

	uart_poll_in   uart_poll_out

运行
=========================
按下开发板复位按钮，运行程序




