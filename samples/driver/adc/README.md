# 简介  
用adc读取模拟信号。通过采集gpio-b8,gpio-b9三个通道的模拟信号，然后将其转换为数字信号并通过串口打印出来。


# 构建步骤
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/adc/ -p


# 功能说明
1.获取ADC设备的句柄
```const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);```

2.遍历配置每个ADC通道

3.设置每个ADC通道的配置信息
```adc_channel_setup(dev_adc, &channel_cfg);```

4.读取每个ADC通道的模拟信号
```err = adc_read(dev_adc, &sequence);```

5.将读取的模拟电压信号转换为毫伏值
```adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, ADC_RESOLUTION,&mv_value);```

6.通过串口打印每个ADC通道的模拟信号


# 运行
按下开发板复位按钮，运行程序





