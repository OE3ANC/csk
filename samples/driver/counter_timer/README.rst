
简介
===================
本示例演示使用dual_timer 和 gpt定时器来实现定时器，10s后，停止定时器，12s后在main函数中重新启用定时器。

构建步骤
======================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/counter_timer/ -p

功能说明
=======================
1.获取ADC设备的句柄

::

    counter_dev = DEVICE_DT_GET(DT_NODELABEL(gptchannel0));

2.配置 counter_top_cfg 结构体

::

        top_cfg.flags = 0;
	top_cfg.ticks = counter_us_to_ticks(counter_dev, DELAY_US);
	top_cfg.callback = test_counter_interrupt_fn;
	top_cfg.user_data = NULL;

3.设置计数器的阈值

::

    err = counter_set_top_value(counter_dev, &top_cfg);

4.无限循环中，每次将 g_counter 置零，然后启动计数器

::

    	while (1)
	{
		g_counter = 0;
		counter_start(counter_dev);
		k_msleep(12000);
		printk("restart\n");
	}


5.通过串口打印计数

运行
========================
按下开发板复位按钮，运行程序,可通过串口查看打印结果
