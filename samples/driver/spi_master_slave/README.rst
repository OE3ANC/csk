简介
=================
使用芯片上的一组SPI引脚往外发送数据，可以设置主模式或者从模式，可通过示波器或者逻辑分析仪去抓取SPI数据。
以下以主模式进行讲解

构建步骤
=================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/spi_master_slave/ -p

功能说明
================
1.从设备树获取SPI设备spi1的设备指针

::

   spi = DEVICE_DT_GET(DT_NODELABEL(spi1));

2. 配置SPI为主设备模式，设置数据位为8位，最低位优先，频率设置为10 MHz
::

   spi_cfg.operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB;
   spi_cfg.frequency = 10 * 1000000UL;

3. 分配内存以存储接收缓冲区的数据
::

   rx_buff = k_malloc(BUF_SIZE);


4. 在无限循环中，周期性地发送和接收数据，并在每次发送后暂停100毫秒
::

   while (1) {
    k_msleep(100);
    memset(rx_buff, 0, BUF_SIZE);
    spi_transceive(spi_master, &spi_cfg, &tx, &rx);
    printk("[主设备]接收:");
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        printk("0x%02x ", rx_buff[i]);
    }
    printk("\n");
   }

运行
=====================
按下开发板复位按钮，运行程序，观察示波器或逻辑分析仪抓取的数据。

