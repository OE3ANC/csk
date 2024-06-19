简介
============
演示如何使用 MCU 的 SD 卡功能，通过写数据到 SD 卡，读取 SD 卡数据，并观察串口信息。

构建步骤
==============
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/sdmmc/ -p


功能说明
==============
1.从设备树中获取 SD 卡设备

::

    static const char *disk_pdrv = CONFIG_SDMMC_VOLUME_NAME;

2.SD 卡设备初始化

::

    ret = disk_access_init(disk_pdrv);

3.拿到 SD 卡的接口信息

::

    ret = disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &cmd_buf);

4.分配数据发送缓存空间

::

    wbuf = (uint8_t *)k_malloc(NUM_SECTORS * disk_sector_size);

5.写数据

::

    ret = disk_access_write(disk_pdrv, wbuf, 1, NUM_SECTORS);

6.分配数据接收缓存空间

::

    rbuf = (uint8_t *)k_malloc(NUM_SECTORS * disk_sector_size);

7.读数据

::

    ret = disk_access_read(disk_pdrv, rbuf, 1, NUM_SECTORS);

8.判断数据，打印结果

运行
================
按下开发板复位按钮，运行程序，可观察串口信息。