简介
=======================
演示如何使用扩展 MCU 的 PWM 功能。运行该代码后, 会在开发板的 EXT_PD2 引脚输出频率为1 HZ,占空比为50%的波形。

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/exmcu_pwm/ -p

功能说明

1.从设备树中定义的 motor_pwm 节点获取 PWM 设备的配置信息

::

	const struct pwm_dt_spec spec = PWM_DT_SPEC_GET(DT_NODELABEL(motor_pwm));

2.检查 PWM 设备是否就绪

::

	device_is_ready()

3.调用 _pwm_set 函数设置 PWM 的周期和脉冲宽度为周期的一半，产生 50% 的占空比

::
    
    r = _pwm_set(spec.period, spec.period / 2);

运行
======================
按下开发板复位按钮，运行程序，可用示波器观察 EXT_PD2 引脚输出的波形。




