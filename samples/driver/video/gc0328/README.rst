简介
=====================
展示了如何使用视频设备驱动接口来配置和捕获视频数据流,基于gc0328摄像头

构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit .sdk/csk/samples/driver/video/gc0328/ -p

功能说明
===================
1.从设备树中获取设备并初始化视频设备
	
::

    video = DEVICE_DT_GET(DT_NODELABEL(dvp));

2.获取视频设备的能力

::

	video_get_caps(video, VIDEO_EP_OUT, &caps)

3.设置视频格式

::

	video_set_format(video, VIDEO_EP_OUT, &fmt)

4.分配视频缓冲区并将其加入捕获队列

::

	buffers[i] = video_buffer_alloc(bsize)

5.开始视频捕获

::

	video_start(video, VIDEO_EP_OUT)

6.取出视频帧判断是否需要下采样并发送

::

	frame_send(vbuf->buffer, fmt.width, fmt.height, 0);

7.打印日志

::

	LOG_INF("Got frame %u! size: %u; timestamp %u ms fps %d", frame++,vbuf->bytesused, vbuf->timestamp, calculate_fps(vbuf))

运行
==========================
按下开发板复位按钮，运行程序