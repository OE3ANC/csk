简介
======================
控制 gpio-a7引脚，每1s翻转一次，通过示波器可以看到翻转的波形。

构建步骤
====================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/gpio/ -p

功能说明
======================
1.获取与别名 gpioa7 相关联的设备树节点
	
::
  
    #define GPIOA_7_NODE DT_ALIAS(gpioa7)

2.从设备树中获取 gpioa7 设备

::

	static const struct gpio_dt_spec gpioa7 = GPIO_DT_SPEC_GET(GPIOA_7_NODE, gpios);

3.配置 gpioa7 为输出模式并设置为活跃状态

::

	gpio_pin_configure_dt(&gpioa7, GPIO_OUTPUT_ACTIVE);

4.交替调用 gpio_a7_enable 和 gpio_a7_disenable 函数，使 gpioa7 引脚的电平每秒钟切换一次

运行
===========================
按下开发板复位按钮，运行程序,观察示波器的变化。




