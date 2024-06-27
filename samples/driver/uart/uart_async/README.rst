简介
=====================
演示如何使用 UART 进行异步收发

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/uart/uart_async/ -p

功能说明
=====================
1.从设备树中获取设备的句柄

::

    const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

2.UART 回调函数设置

::

    uart_callback_set(uart_dev, uart_async_callback, NULL);

3.启用 UART 接收

::

    uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50 * USEC_PER_MSEC);

4.启动 UART 传输

::

    uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100 * USEC_PER_MSEC);

5.等待传输完成信号量 tx_done，如果超时则记录错误并返回

::

    k_sem_take(&tx_done, K_MSEC(100)) != 0

6.等待接收准备信号量 rx_rdy，如果超时则记录错误并返回。

::

    k_sem_take(&rx_rdy, K_MSEC(100)) != 0

7.比较并打印结果

运行
======================
按下开发板复位按钮，连接 PA12 和 PA13 引脚，可通过串口查看输出结果

