
/*
* SPDX-License-Identifier: Apache-2.0
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief 加锁整片flash，阻止写入和擦除操作。
 * 
 * @return 0 : 成功，flash已经被锁住
 * @retval 非0 : 没有成功
 * 
 */
extern int flash_lock(const struct device *dev);

/**
 * @brief 解锁整片flash，可以写入和擦除操作。
 * 
 * @return 0 : 成功，flash已经被解锁
 * @retval 非0 : 没有成功
 * 
 */
extern int flash_unlock(const struct device *dev);

/**
 * @brief 获取flash是否被锁住。
 * 
 * @return 0 : 没有被锁
 * @retval 1 : 被锁住
 * 
 */
extern int flash_get_lock_status(const struct device *dev);

#ifdef __cplusplus
}
#endif
