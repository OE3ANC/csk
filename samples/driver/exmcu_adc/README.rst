简介
=====================
演示如何调用adc接口来实现扩展MCU的ADC功能。运行该应用后,会间隔采集EXT_PA2引脚上的电压,并通过日志输出。

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/exmcu_adc/ -p

功能说明
=====================
1.获取ADC设备的句柄

::

	const struct device *dev_adc = DEVICE_DT_GET(ADC_NODE);

2.遍历配置每个ADC通道

3.设置每个ADC通道的配置信息

::

    adc_channel_setup(dev_adc, &channel_cfg);

4.读取每个ADC通道的模拟信号

::

    err = adc_read(dev_adc, &sequence);

5.将读取的模拟电压信号转换为毫伏值

::

    adc_raw_to_millivolts(adc_vref, ADC_GAIN_1, ADC_RESOLUTION,&mv_value);

6.通过串口打印每个ADC通道的模拟信号

运行
====================
按下开发板复位按钮，运行程序,观察串口打印信息。




