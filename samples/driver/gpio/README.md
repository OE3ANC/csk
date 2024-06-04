# 简介
控制 gpio-a1引脚，每1s翻转一次，通过示波器可以看到翻转的波形。
# 构建步骤
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/gpio/ -p


# 功能说明
1.获取与别名 gpioa1 相关联的设备树节点
```	#define GPIOA_1_NODE DT_ALIAS(gpioa1)```

2.从设备树中获取 gpioa1 设备
```	static const struct gpio_dt_spec gpioa1 = GPIO_DT_SPEC_GET(GPIOA_1_NODE, gpios);```

3.配置 gpioa1 为输出模式并设置为活跃状态
```	gpio_pin_configure_dt(&gpioa1, GPIO_OUTPUT_ACTIVE);```

4.交替调用 gpio_a1_enable 和 gpio_a1_disenable 函数，使 gpioa1 引脚的电平每秒钟切换一次

# 运行
按下开发板复位按钮，运行程序