简介
=====================
演示验证闪存的读写和擦除功能


构建步骤
=====================
lisa zep build -b csk6_duomotai_devkit ./samples/driver/nor_flash/ -p


功能说明
=====================
1. 从设备树中获取设备的句柄
::

    const struct device *flash_dev = DEVICE_DT_GET(DT_INST(0, listenai_csk_flash_controller));

2. 检查设备是否就绪
::

    if (!device_is_ready(flash_dev)) {
        printf("Litenai CSK6 flash driver was not found!\n");
        return;
    }

3. 启用闪存锁定（根据配置）
::

    flash_lock(flash_dev);

4. 尝试写入数据并检查锁定状态
::

   flash_write(flash_dev, FLASH_TEST_OFFSET, &test_data, sizeof(int)) != 0

5.解锁闪存并再次写入数据

::

   flash_unlock(flash_dev);
   flash_write(flash_dev, FLASH_TEST_OFFSET, &test_data, sizeof(int)) != 0

6.Test 1:擦除页并检查结果

::

   flash_erase(flash_dev, FLASH_TEST_OFFSET, FLASH_PAGE_SIZE) != 0

7.Test 2:写入和读取数据1，验证结果

::

   flash_write(flash_dev, offset, &buf_array_1[i], sizeof(uint32_t)) != 0
   flash_read(flash_dev, offset, &buf_word, sizeof(uint32_t)) != 0

8.Test 3:擦除多页数据

::

   flash_erase(flash_dev, offset, FLASH_PAGE_SIZE * 4) != 0

9.Test 4:写入和读取数据2，验证结果

10.Test 5:再次擦除页

11.Test 6:非字对齐写入及验证

::

   flash_write(flash_dev, offset, &buf_array_3[i], sizeof(uint32_t)) != 0

12. 获取和打印页布局信息
::

   rc = flash_get_page_info_by_offs(flash_dev, FLASH_TEST_OFFSET2, &info);
   rc = flash_get_page_info_by_idx(flash_dev, FLASH_TEST_PAGE_IDX, &info);

13. 获取并打印写块大小
::

    printf("   write-block-size = %u\n", flash_get_write_block_size(flash_dev));


14. 从内部闪存区域写入数据并验证

运行
======================
按下开发板复位按钮，可通过串口查看输出结果

