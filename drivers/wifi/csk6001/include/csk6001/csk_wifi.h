/*
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "commands.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: Add WIFI state machine transition diagram */

/**
 * @brief CSK WiFi operation(API) results structure
 */
typedef enum {
    CSK_WIFI_SUCCESS,                   /* WIFI operation successful */
    CSK_WIFI_ERR_STA_FAILED,            /* Unknown Station exception, will be raised in Station mode. TODO: remove it */
    CSK_WIFI_ERR_AP_FAILED,             /* Unknown SoftAP exception, will be raised in SoftAP mode. TODO: remove it */
    CSK_WIFI_ERR_AP_NOT_FOUND,          /* AP ssid not found, will be raised in Station mode */
    CSK_WIFI_ERR_INVALID_PASSWORD,      /* Invalid WIFI password, will be raised in Station mode */
} csk_wifi_result_t;                    /* WIFI API result type */

/**
 * @brief CSK WiFi events structure
 */
typedef enum {
    CSK_WIFI_EVT_STA_CONNECTED =        BIT(0),     /* WIFI-Station connected event bit */
    CSK_WIFI_EVT_STA_DISCONNECTED =     BIT(1),     /* WIFI-Station disconnected event bit */
    CSK_WIFI_EVT_AP_STARTED =           BIT(2),     /* WIFI-SoftAP stared event bit */
    CSK_WIFI_EVT_AP_STOPPED =           BIT(3),     /* WIFI-SoftAP stopped event bit */
    CSK_WIFI_EVT_AP_STACONNECTED =      BIT(4),     /* WIFI-SoftAP station connected event bit */
    CSK_WIFI_EVT_AP_STADISCONNECTED =   BIT(5),     /* WIFI-SoftAP station disconnected event bit */
} csk_wifi_event_t;

/**
 * WIFI event handler type (will be invoked in `csk_wifi_event_cb_t`)
 */
typedef void(*wifi_event_cb_t) (csk_wifi_event_t event, void *arg);

/**
 * @brief CSK WIFI callback structure
 *
 * Used to register a callback in the csk_wifi driver instance callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct csk_wifi_event_cb.
 * Beware such structure should not be allocated on stack.
 *
 * TODO: add a callback initializer
 */
typedef struct csk_wifi_event_cb {
    /**
     * This is meant to be used in the driver core and the user should not
	 * touch it (see csk_wifi_api.c).
	 */
    sys_snode_t node;

    /**
     * This function will be called while WIFI event occurred
     */
    wifi_event_cb_t handler;

    /**
     * Wifi events mask, see `csk_wifi_event_t`
     */
    uint32_t events;
    void *arg;
} csk_wifi_event_cb_t;

/**
 * @brief Add an wifi event callback
 * @param wifi_event_cb[in] Application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 */
int csk_wifi_add_callback(csk_wifi_event_cb_t *wifi_event_cb);

#ifdef CONFIG_CSK_WIFI_STATION
/**
 * @brief Start a WIFI connection request as a WiFi station device
 *
 * This function will send a WiFi station connect request to the driver core
 * and the WiFi driver thread will start the real WiFi connection request. When
 * the WiFi station connected, CSK_WIFI_EVT_STA_CONNECTED event will be triggered
 * and WiFi event handler will be called (if have).
 *
 * @note This function could only call while WIFI is in Station disconnected status
 *
 * @param sta_config[in] Pointer to WIFI Station configuration structure
 * @param result[out] Pointer to WIFI operation result
 * @param timeout[in] How long to try to connect WIFI Access Point
 *
 * @return 0 if successful, negative errno code on failure.
 */
int csk_wifi_sta_connect(const esp_hosted_sta_config_t *sta_config, csk_wifi_result_t *result, k_timeout_t timeout);

/**
 * @brief Start a WIFI disconnection request
 *
 * This function will send a WiFi station disconnect request to the driver core
 * and the WiFi driver thread will start the real WiFi disconnection request. When
 * the WiFi station disconnected, CSK_WIFI_EVT_STA_DISCONNECTED event will be triggered
 * and WiFi event handler will be called (if have).
 *
 * @note This function could only call while WIFI is in Station connected status
 *
 * @param result[out] Pointer to WIFI operation result
 * @param timeout[in] How long to try to disconnect from WIFI Access Point
 *
 * @return 0 if successful, negative errno code on failure.
 */
int csk_wifi_sta_disconnect(csk_wifi_result_t *result, k_timeout_t timeout);
#endif

#ifdef CONFIG_CSK_WIFI_SOFTAP
/**
 * @brief Set up WIFI Access Point
 *
 * This function will send a WIFI Access Point start request to the driver core
 * and the WiFi driver thread will call the real WIFI SoftAP startup function. When
 * the WIFI Access Point startup, CSK_WIFI_EVT_AP_STARTED event will be triggered
 * and WiFi event handler will be called (if have).
 *
 * @note This function could only call while WIFI is in SoftAP stopped status
 *
 * @param ap_config[in] Pointer to WIFI SoftAP configuration structure
 * @param result[out] Pointer to operation result
 * @param timeout[in] How long to try to set up WIFI Access Point
 *
 * @return 0 if successful, negative errno code on failure.
 */
int csk_wifi_ap_start(const esp_hosted_softap_config_t *ap_config, csk_wifi_result_t *result, k_timeout_t timeout);

/**
 * @brief Stop WIFI Access Point
 *
 * This function will send a WIFI Access Point stop request to the driver core
 * and the WiFi driver thread will call the real WIFI SoftAP stop function. When
 * the WIFI Access Point stopped, CSK_WIFI_EVT_AP_STOPPED event will be triggered
 * and WiFi event handler will be called (if have).
 *
 * @note This function could only call while WIFI is in SoftAP started status
 *
 * @param result[out] Pointer to operation result
 * @param timeout[in] How long to try to stop WIFI Access Point
 *
 * @return 0 if successful, negative errno code on failure.
 */
int csk_wifi_ap_stop(csk_wifi_result_t *result, k_timeout_t timeout);
#endif

#ifdef __cplusplus
}
#endif
