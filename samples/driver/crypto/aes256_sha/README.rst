
简介
===================
本示例演示使用SHA-256算法对输入数据进行哈希处理，并验证计算结果是否正确

构建步骤
======================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/crypto/aes256_sha/ -p

功能说明
=======================
1.从设备树中获取设备

::

    const struct device *dev = DEVICE_DT_GET(CRYPTO_DRV_NAME);

2.调用函数进行哈希操作

::

   hash_mode(dev);

3.开始哈希会话

::

   ret = hash_begin_session(dev, &ctx, CRYPTO_HASH_ALGO_SHA256);


4.计算哈希值

::

   ret = hash_compute(&ctx, &pkt);

5.验证哈希结果

::

   memcmp(crypto_sha_result_data_8w, sha256_results[6], 32)

6.结束哈希会话

::

   hash_free_session(dev, &ctx);

运行
========================
按下开发板复位按钮，运行程序,可通过串口查看打印结果





