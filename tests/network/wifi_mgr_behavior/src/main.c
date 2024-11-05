#include "wifi_manager/wifi_manager.h"
#include "zephyr/sys/printk.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys_clock.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(lib_wif_mgr_test, LOG_LEVEL_DBG);

extern void test_lib_wifi_mgr_enable_connect_cb(void);
extern void test_lib_wifi_mgr_disable_connect_cb(void);
extern wifi_mgr_connection_status_t
test_lib_wifi_mgr_get_sta_connect_status(k_timeout_t wait_time_ms);
extern wifi_mgr_connection_status_t
test_lib_wifi_mgr_get_sta_connect_status_while_no_register_callback(int timeout_seconds);
extern void test_wifi_mgr_storage_save_ap_assert(wifi_mgr_sta_config_t *sta_config);
extern void test_lib_wifi_mgr_sta_disable_and_call_api_then_enable(void);
extern void test_lib_wifi_mgr_auto_connect_then_disable_finally_enable(void);
extern void test_wifi_mgr_storage_delete_all_ap(void);
extern void test_lib_wifi_mgr_force_save_ap(void);
extern int test_lib_wifi_mgr_sta_get_connected_info(wifi_mgr_sta_config_t *got_sta_info);
extern void test_wifi_mgr_storage_save_ap_force(wifi_mgr_sta_config_t *in_sta_config);
extern void wifi_auto_connect_start_assert(int interval_ms);
extern void wifi_auto_connect_stop_assert();
extern void test_lib_wifi_mgr_clear_connect_status(void);
extern void test_lib_wifi_mgr_is_connecting_assert(k_timeout_t timeout);
extern void test_lib_wifi_mgr_is_disconnected_assert(k_timeout_t timeout);
extern int test_lib_wifi_mgr_connection_failed_assert(k_timeout_t timeout);

void test_lib_wifi_mgr_auto_connect_to_ap_with_correct_ssid_and_pwd_then_disable_finally_enable(
	void);

static wifi_mgr_sta_config_t correct_sta_config = {
	.ssid = CONFIG_CORRECT_WIFI_SSID,
	.pwd = CONFIG_CORRECT_WIFI_PASSWORD,
	.encryption_mode = CSK_WIFI_AUTH_WPA2_PSK,
};

static wifi_mgr_sta_config_t correct_ssid_wrong_pwd_sta_config = {
	.ssid = CONFIG_CORRECT_WIFI_SSID,
	.pwd = CONFIG_WRONG_WIFI_PASSWORD,
	.encryption_mode = CSK_WIFI_AUTH_WPA2_PSK,
};

static wifi_mgr_sta_config_t wrong_sta_config = {
	.ssid = CONFIG_WRONG_WIFI_SSID,
	.pwd = CONFIG_WRONG_WIFI_PASSWORD,
	.encryption_mode = CSK_WIFI_AUTH_WPA2_PSK,
};

K_SEM_DEFINE(async_scan_done, 0, 1);
K_QUEUE_DEFINE(scan_memory_leak);

#define CONTINUE_MEMORY_LEAK_TIMES 5

extern struct heap_t _system_heap;

static struct sys_memory_stats last_stat = {0};
static int memory_leak_count = 0;
static int memory_leaked_sum = 0;

static void wifi_disconnect_assert(void)
{
	int ret = 0;
	wifi_mgr_connection_status_t sta_status = 0;

	ret = wifi_mgr_sta_disconnect(false);
	zassert_equal(ret, 0, "disconnect failed");
	if (IS_ENABLED(CONFIG_CSK_WIFI_XR819S)) {
		ret = wifi_mgr_sta_disconnect(false);
		zassert_not_equal(
			ret, 0,
			"disconnect shouldn't return 0 if device has disconnected before. ret: %d",
			ret);
	}
	sta_status = wifi_mgr_sta_get_status();
	zassert_equal(sta_status, WIFI_MGR_STA_DISCONNECTED, "disconnect failed, actual status: %d",
		      sta_status);
	test_lib_wifi_mgr_clear_connect_status();
}

static bool check_memory_leak(void)
{
	bool ret = false;

	if (last_stat.free_bytes == 0 && last_stat.allocated_bytes == 0) {
		sys_heap_runtime_stats_get((struct sys_heap *)(&_system_heap), &last_stat);
		ret = false;
	} else {
		struct sys_memory_stats stat;
		sys_heap_runtime_stats_get((struct sys_heap *)(&_system_heap), &stat);
		if (stat.free_bytes < last_stat.free_bytes) {
			ret = true;
		} else {
			ret = false;
		}
		memory_leaked_sum += (last_stat.free_bytes - stat.free_bytes);
		last_stat.allocated_bytes = stat.allocated_bytes;
		last_stat.free_bytes = stat.free_bytes;
	}
	return ret;
}

static inline int get_memory_leaked_num(void)
{
	return memory_leaked_sum;
}

static void memleak_check_scandone_cb(wifi_mgr_scan_info_t *aps_info, int ap_num, void *arg)
{
	if (check_memory_leak()) {
		memory_leak_count++;
		if (memory_leak_count >= CONTINUE_MEMORY_LEAK_TIMES) {
			int memory_leaked = get_memory_leaked_num();
			k_queue_append(&scan_memory_leak, &memory_leaked);
		}
	} else {
		LOG_DBG("reset memory leak count");
		memory_leak_count = 0;
	}
}

static void async_scan_done_cb(wifi_mgr_scan_info_t *aps_info, int ap_num, void *arg)
{
	if (ap_num > 0) {
		k_sem_give(&async_scan_done);
	}
}

static void *test_lib_wifi_mgr_setup(void)
{
	int ret;

	ret = wifi_mgr_init();
	zassert_equal(ret, 0, "wifi mgr init failed, ret: %d", ret);

	test_wifi_mgr_storage_delete_all_ap();

	test_lib_wifi_mgr_enable_connect_cb();

	return NULL;
}

static void get_connected_info_assert(wifi_mgr_sta_config_t *expect_sta_info)
{
	zassert_true((expect_sta_info != NULL), "expect_sta_info cannot be NULL\n");

	int ret = 0;
	wifi_mgr_sta_config_t got_sta_info = {0};

	ret = test_lib_wifi_mgr_sta_get_connected_info(&got_sta_info);
	zassert_equal(ret, 0, "get connected info failed, ret: %d \n", ret);

	zassert_mem_equal(got_sta_info.ssid, expect_sta_info->ssid, sizeof(correct_sta_config.ssid),
			  "connected ssid are different to target one\n");
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_async_connect_then_async_disconnect)
{
	int ret = 0;
	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;

	test_lib_wifi_mgr_clear_connect_status();
	ret = wifi_mgr_sta_connect(&correct_sta_config, true);
	zassert_equal(ret, 0, "wifi connect failed, ret: %d", ret);

	test_lib_wifi_mgr_is_connecting_assert(K_SECONDS(3));

	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED, "Failed to connect target wifi");

	get_connected_info_assert(&correct_sta_config);

	wifi_mgr_sta_disconnect(true);

	test_lib_wifi_mgr_is_disconnected_assert(K_SECONDS(5));
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_async_scan_and_auto_connect_then_disconnect)
{
	int ret = 0;
	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;

	ret = wifi_mgr_add_scan_done_cb(async_scan_done_cb, NULL);
	zassert_equal(ret, 0, "add async scan done callback failed, ret: %d", ret);

	ret = wifi_mgr_scan_ap(NULL, true);
	zassert_equal(ret, 0, "async scan ap failed, ret: %d", ret);

	ret = k_sem_take(&async_scan_done, K_MSEC(3000));
	zassert_equal(ret, 0, "There are no async scan done event occur, ret: %d", ret);

	wifi_mgr_storage_save_ap_force(&correct_sta_config);

	wifi_auto_connect_start_assert(3000);

	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to connect target wifi, connect_status: %d \n", connect_status);
	wifi_mgr_storage_delete_ap(&correct_sta_config);
	wifi_disconnect_assert();
	wifi_auto_connect_stop_assert();
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_sta_connect_with_correct_ssid_and_pwd_then_disconnect)
{
	int ret = 0;

	ret = wifi_mgr_sta_connect(&correct_sta_config, false);
	zassert_equal(ret, 0, "wifi connect failed, ret: %d", ret);

	get_connected_info_assert(&correct_sta_config);

	wifi_disconnect_assert();
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_sta_connect_with_correct_ssid_but_wrong_pwd)
{
	int ret = 0;
	int reason = 0;

	ret = wifi_mgr_sta_connect(&correct_ssid_wrong_pwd_sta_config, false);
	reason = test_lib_wifi_mgr_connection_failed_assert(K_MSEC(1000));
	zassert_not_equal(ret, 0, "wifi shouldn't connect success, ret: %d", ret);
	zassert_true(reason < 0, "Connection failed, reason: %d", reason);
	if (ret == 0) {
		wifi_mgr_sta_disconnect(false);
	}
}

ZTEST(lib_wifi_mgr_test,
      test_lib_wifi_mgr_auto_connect_to_ap_with_correct_ssid_and_pwd_then_disable_finally_enable)
{
	int ret = 0;
	wifi_mgr_connection_status_t sta_status = 0;

	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;

	test_wifi_mgr_storage_delete_all_ap();
	test_wifi_mgr_storage_save_ap_assert(&correct_sta_config);

	test_wifi_mgr_storage_save_ap_force(&correct_sta_config);

	wifi_auto_connect_start_assert(1000);

	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to connect target wifi, connect_status: %d \n", connect_status);

	/* Disonnect device manually */
	wifi_disconnect_assert();

	/* Make sure WiFi manager will reconnect to device automatically */
	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to reconnect target wifi, connect_status: %d \n", connect_status);

	/* Disonnect device manually */
	wifi_disconnect_assert();
	wifi_mgr_auto_connect_stop();

	wifi_mgr_storage_delete_ap(&correct_sta_config);

	ret = wifi_mgr_sta_disable();
	zassert_equal(ret, 0, "wifi_mgr_sta_disable failed, ret: %d", ret);

	sta_status = wifi_mgr_sta_get_status();
	zassert_equal(sta_status, WIFI_MGR_STA_DISCONNECTED, "disconnect failed, actual status: %d",
		      sta_status);
	wifi_auto_connect_stop_assert();

	ret = wifi_mgr_sta_enable();
	zassert_equal(ret, 0, "wifi_mgr_sta_enable failed, ret: %d", ret);
}

ZTEST(lib_wifi_mgr_test,
      test_lib_wifi_mgr_auto_connect_to_ap_without_callback_then_disconnect_and_wait_reconnect_finally_disconnect)
{
	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;

	/* Remove connection callback */
	test_lib_wifi_mgr_disable_connect_cb();

	test_wifi_mgr_storage_delete_all_ap();

	test_wifi_mgr_storage_save_ap_assert(&correct_sta_config);

	test_wifi_mgr_storage_save_ap_force(&correct_sta_config);

	wifi_auto_connect_start_assert(1000);

	TC_PRINT("%s: %u \n", __FUNCTION__, __LINE__);

	connect_status = test_lib_wifi_mgr_get_sta_connect_status_while_no_register_callback(10);
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to connect target wifi, connect_status: %d \n", connect_status);

	TC_PRINT("%s: %u \n", __FUNCTION__, __LINE__);

	/* Disonnect device manually */
	wifi_disconnect_assert();

	/* Make sure WiFi manager will reconnect to device automatically */
	connect_status = test_lib_wifi_mgr_get_sta_connect_status_while_no_register_callback(10);
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to reconnect target wifi, connect_status: %d \n", connect_status);

	wifi_mgr_storage_delete_ap(&correct_sta_config);
	wifi_auto_connect_stop_assert();
	wifi_disconnect_assert();

	test_lib_wifi_mgr_enable_connect_cb();
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_auto_connect_then_scan_finally_stop_auto_connect)
{
	int ret = 0;
	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;
	test_wifi_mgr_storage_save_ap_assert(&correct_sta_config);
	wifi_mgr_connection_status_t sta_status = WIFI_MGR_STA_MAX;

	test_wifi_mgr_storage_save_ap_force(&correct_sta_config);

	wifi_auto_connect_start_assert(1000);

	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to connect target wifi, connect_status: %d \n", connect_status);

	wifi_auto_connect_stop_assert();
	wifi_mgr_scan_info_t *ap_info = NULL;
	ret = wifi_mgr_scan_ap(&ap_info, false);
	zassert_true(ret > 0, "scan ap failed, ret: %d", ret);
	if (ap_info != NULL) {
		k_free(ap_info);
	}
	sta_status = wifi_mgr_sta_get_status();
	zassert_equal(sta_status, WIFI_MGR_STA_CONNECTED,
		      "wifi_mgr_sta_get_status, actual status: %d", sta_status);

	/* Disonnect device manually */
	wifi_disconnect_assert();

	/* Make sure WiFi manager will not reconnect to device automatically */
	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_not_equal(connect_status, WIFI_MGR_STA_CONNECTED,
			  "shoudn't connect success, connect_status: %d \n", connect_status);

	// wifi_mgr_storage_delete_ap(&correct_sta_config);
	test_wifi_mgr_storage_delete_all_ap();
}

ZTEST(lib_wifi_mgr_test,
      test_lib_wifi_mgr_enable_auto_connect_on_connected_status_finally_disconnect)
{
	int ret = 0;
	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;

	ret = wifi_mgr_sta_connect(&correct_sta_config, false);
	zassert_equal(ret, 0, "wifi connect failed, ret: %d", ret);

	get_connected_info_assert(&correct_sta_config);
	test_wifi_mgr_storage_save_ap_force(&correct_sta_config);

	wifi_auto_connect_start_assert(1000);

	/* Disonnect device manually */
	wifi_disconnect_assert();

	/* Make sure WiFi manager will reconnect to device automatically */
	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(10 * 1000));
	zassert_equal(connect_status, WIFI_MGR_STA_CONNECTED,
		      "Failed to reconnect target wifi, connect_status: %d \n", connect_status);

	wifi_auto_connect_stop_assert();
	wifi_mgr_storage_delete_ap(&wrong_sta_config);

	wifi_disconnect_assert();
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_auto_connect_to_ap_with_wrong_ssid_and_pwd)
{
	wifi_mgr_connection_status_t connect_status = WIFI_MGR_STA_MAX;

	test_wifi_mgr_storage_save_ap_assert(&wrong_sta_config);

	wifi_auto_connect_start_assert(3000);

	TC_PRINT("Found ssid in NVS storage, ssid: %s, password: %s \n", wrong_sta_config.ssid,
		 wrong_sta_config.pwd);

	connect_status = test_lib_wifi_mgr_get_sta_connect_status(K_MSEC(1000));
	zassert_not_equal(connect_status, WIFI_MGR_STA_CONNECTED,
			  "Failed to connect target wifi, connect_status: %d", connect_status);

	wifi_mgr_storage_delete_ap(&wrong_sta_config);

	wifi_auto_connect_stop_assert();
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_check_memory_leak_for_scan)
{
	void *queue_get_data = NULL;
	int interval_ms = 100;

	test_wifi_mgr_storage_save_ap_assert(&wrong_sta_config);

	wifi_mgr_add_scan_done_cb(memleak_check_scandone_cb, NULL);

	wifi_auto_connect_start_assert(interval_ms);

	queue_get_data = k_queue_get(&scan_memory_leak, K_SECONDS(5));

	wifi_mgr_remove_scan_done_cb(memleak_check_scandone_cb);
	wifi_auto_connect_stop_assert();

	zassert_equal(queue_get_data, NULL, "It shouldn't memory leak, memory_leaked_sum: %d",
		      *((int *)(queue_get_data)));
}

static void test_lib_wifi_mgr_teardown(void *fixture)
{
	test_lib_wifi_mgr_disable_connect_cb();
	test_wifi_mgr_storage_delete_all_ap();
}

ZTEST(lib_wifi_mgr_test, test_lib_wifi_mgr_storage_api)
{
	test_wifi_mgr_storage_delete_all_ap();
	test_wifi_mgr_storage_save_ap_force(&correct_sta_config);
}

ZTEST_SUITE(lib_wifi_mgr_test, NULL, test_lib_wifi_mgr_setup, NULL, NULL, test_lib_wifi_mgr_teardown);
