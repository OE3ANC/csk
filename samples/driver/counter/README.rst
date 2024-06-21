简介
=================
演示使用 RTC 定时器来实现定时，1s后获取定时器计数，设置闹钟为10s后触发回调。

构建步骤
=================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/counter/ -p

功能说明
=================
1.获取ADC设备的句柄

::

    rtc = DEVICE_DT_GET(DT_NODELABEL(rtc1));

2.检查设备就绪状态

::

    device_is_ready(rtc)

3.读取和打印RTC的最大计数值（顶值）

::

    uint32_t rtc_top_value = counter_get_top_value(rtc);

4.启动计数器并在1秒后读取计数值

::
    
    counter_start(rtc);

5.设置闹钟并指定回调函数

::
    
    sec_counter_callback(const struct device *dev, uint8_t id, uint32_t ticks, void *ud)

6.在回调函数中打印信息

运行
========================
按下开发板复位按钮，运行程序，观察串口打印信息。



