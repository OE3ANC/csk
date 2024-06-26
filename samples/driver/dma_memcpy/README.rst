简介
=====================
演示如何使用Zephyr RTOS的DMA功能进行高效的内存拷贝。运行该代码后，将比较DMA和标准memcpy的性能。

构建步骤
=====================

lisa zep build -b csk6_duomotai_devkit ./samples/driver/dma/ -p

功能说明
=====================
1. 从设备树中获取DMA设备

::

    const struct device *dma = DEVICE_DT_GET(DT_NODELABEL(dma0));

2. 配置并启动DMA进行内存拷贝

::

    test_dma_copy(dma, src_buffer, dst_buffer, BUFFER_SIZE);

3. 使用标准的memcpy进行内存拷贝

::

    test_std_memcpy(src_buffer, dst_buffer, BUFFER_SIZE);

4. 输出DMA和memcpy操作的耗时

5. 循环执行各种内存拷贝操作

运行
======================
按下开发板复位按钮，运行程序。程序将间隔性地执行DMA和标准memcpy操作，并通过串口输出各自的耗时数据，以此来比较两者的性能。


