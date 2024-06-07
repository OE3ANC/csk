# 简介
演示了如何使用Zephyr RTOS中的看门狗定时器（WDT）来监控系统状态，并在系统异常时进行重置。

# 构建步骤
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/wdt/ -p

# 功能说明
1.从设备树获取看门狗设备并进行初始化
```	wdt = DEVICE_DT_GET(DT_NODELABEL(wdt));```

2.看门狗超时配置，并启用回调函数
```wdt_config.callback = wdt_callback;```


3.装载配置并获取通道ID
```	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);```

4.设置看门狗实例
```	err = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);```

5.先喂食看门狗避免系统重置
```	wdt_feed(wdt, wdt_channel_id); ```

6.等待系统重置


# 运行
按下开发板复位按钮，运行程序