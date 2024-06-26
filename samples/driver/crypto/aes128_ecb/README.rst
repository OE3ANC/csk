
简介
===================
本示例演示使用AES-128 ECB模式进行加密和解密的示例操作

构建步骤
======================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/crypto/aes128_ecb/ -p

功能说明
=======================
1.从设备树中获取设备

::

    const struct device *dev = DEVICE_DT_GET(CRYPTO_DRV_NAME);

2.调用函数进行加密解密操作

::

   ecb_mode(dev);

3.首先初始化 cipher_ctx 和 cipher_pkt 结构体。


4.开始加密会话

::

   cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES, CRYPTO_CIPHER_MODE_ECB,CRYPTO_CIPHER_OP_ENCRYPT)


5.执行加密操作

::

   cipher_block_op(&ini, &encrypt)

6.验证加密结果

::

   memcmp(encrypt.out_buf, ecb_ciphertext, sizeof(ecb_ciphertext))

7.结束加密会话

::

   cipher_free_session(dev, &ini);

8.开始解密会话

::

   cipher_begin_session(dev, &ini, CRYPTO_CIPHER_ALGO_AES, CRYPTO_CIPHER_MODE_ECB,CRYPTO_CIPHER_OP_DECRYPT)

9.执行解密操作

::

   cipher_block_op(&ini, &decrypt)

10.验证解密结果

::

   memcmp(decrypt.out_buf, ecb_plaintext, sizeof(ecb_plaintext))

11.结束解密会话 

::

   cipher_free_session(dev, &ini);

运行
========================
按下开发板复位按钮，运行程序,可通过串口查看打印结果


