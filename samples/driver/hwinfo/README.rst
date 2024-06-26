简介
=====================
演示如何获取设备的唯一标识符和芯片类型，并将这些信息打印到控制台

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/hwinfo/ -p

功能说明
=====================
1.硬件获取设备 ID

::

    length = hwinfo_get_device_id((uint8_t *)&buffer, sizeof(buffer));

2.检查获取的长度

::

    if(length != sizeof(buffer)){
        printk("length error (%d)\n", length);
        return -1;
    }

运行
======================
按下开发板复位按钮，运行程序，在控制台中可以看到打印信息。







