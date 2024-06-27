简介
=====================
演示如何生成随机数。运行该代码后，将获取一组随机数据并打印出来。

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/entropy/ -p

功能说明
=====================
1. 从设备树中获取设备

::

    dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_entropy));

2. 获取熵数据

::

    ret = entropy_get_entropy(dev, entropy_buf, ENTROPY_LEN_MAX);


3. 打印熵数据

::

    dump_entropy(entropy_buf, ENTROPY_LEN_MAX);

运行
======================
按下开发板复位按钮，运行程序。将打印出获取的随机数据。
