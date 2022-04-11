#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <kernel.h>
#include <net/ethernet.h>
#include <net/net_pkt.h>
#include <net/net_if.h>

#include "spi_drv.h"
#include "netdev_api.h"
#include "hosted/hosted.h"
#include "csk6001/csk_wifi.h"

#define CSK_TAG "CSK WIFI"

#define CSK_WIFI_CHECK(cond, ret_val) ({                                      \
    if (!(cond)) {                                                            \
        printk("[%s] %s(%d)", CSK_TAG, __FUNCTION__, __LINE__);               \
        return (ret_val);                                                     \
    }                                                                         \
})

#ifdef __cplusplus
extern "C" {
#endif

struct csk_wifi_data {
    const struct device *dev;
    struct network_handle *virtual_netif;
    struct net_if *netif;
    uint8_t mac_addr[6];
    union {
        esp_hosted_sta_config_t sta_config;
        esp_hosted_softap_config_t ap_config;
    };
    hosted_event_callback_t hosted_event_cb;
};

typedef enum {
    WIFI_CMD_STA_INIT,
    WIFI_CMD_STA_CONNECT,
    WIFI_CMD_STA_DISCONNECT,
    WIFI_CMD_AP_INIT,
    WIFI_CMD_AP_START,
    WIFI_CMD_AP_STOP,
    WIFI_CMD_SCAN,
} wifi_cmd_t;

typedef struct {
    csk_wifi_result_t result;
    void *arg;
} wifi_ret_msg_t;

typedef struct {
    csk_wifi_scan_info_t *ap_list;
    int count;
} wifi_scan_list_t;

const struct device * wifi_get_dev(wifi_mode_t mode);
int wifi_send_command(const struct device *dev, wifi_cmd_t command, k_timeout_t timeouts);
int wifi_take_result(wifi_ret_msg_t *ret_msg, k_timeout_t timeout);

void wifi_dispatch_callbacks(csk_wifi_event_t events, void *event_data, uint32_t event_data_len);
wifi_auth_mode_t wifi_get_encryption_mode(csk_wifi_encryption_mode_t mode);

#ifdef __cplusplus
}
#endif
