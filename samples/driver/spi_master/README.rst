简介
===============
使用芯片上的一组SPI引脚往外发送数据，可通过示波器或者逻辑分析仪去抓取spi数据。

构建步骤
===============
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/spi_master/ -p

功能说明
===============
1.从设备树获取SPI设备spi0的设备指针

::

    spi = DEVICE_DT_GET(DT_NODELABEL(spi0));

2.配置SPI为主设备模式，设置数据位为8位，最低位优先，频率设置为10 MHz

3.分配内存以存储10个SPI缓冲区的数组

::

    struct spi_buf *tx_package = k_calloc(tx_package_num, sizeof(struct spi_buf));

4.在无限循环中，周期性地发送数据并在每次发送后暂停1000毫秒

::

    spi_write(spi, &spi_cfg, &tx_set);

运行
================
按下开发板复位按钮，运行程序,观察示波器或逻辑分析仪抓取的数据。



