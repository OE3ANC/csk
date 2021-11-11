#include "csk6001/priv/csk_wifi_priv.h"

K_MUTEX_DEFINE(wifi_api_mutex);

static sys_slist_t wifi_callback_list;

void wifi_dispatch_callbacks(csk_wifi_event_t events)
{
    sys_slist_t *list = &wifi_callback_list;
    csk_wifi_event_cb_t *cb, *tmp;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
        if (cb->events & events) {
            cb->handler(cb->events & events, cb->arg);
        }
    }
}

int csk_wifi_add_callback(csk_wifi_event_cb_t *wifi_event_cb)
{
    if (wifi_event_cb == NULL) {
        return -EINVAL;
    }
    sys_slist_prepend(&wifi_callback_list, &wifi_event_cb->node);
    return 0;
}

#ifdef CONFIG_CSK_WIFI_STATION

int csk_wifi_sta_connect(const esp_hosted_sta_config_t *sta_config, csk_wifi_result_t *result, k_timeout_t timeout)
{
    if (sta_config == NULL || result == NULL) {
        return -EINVAL;
    }
    int ret = 0;
    k_mutex_lock(&wifi_api_mutex, K_FOREVER);
    const struct device *dev = wifi_get_dev(WIFI_MODE_STA);
    struct csk_wifi_data *data = dev->data;
    memcpy(&data->sta_config, sta_config, sizeof(esp_hosted_sta_config_t));
    ret = wifi_send_command(dev, WIFI_CMD_STA_CONNECT, K_NO_WAIT);
    if (ret != 0)   goto __exit;
    ret = wifi_take_result(result, timeout);
    if (ret != 0)   goto __exit;
    if (*result != CSK_WIFI_SUCCESS) {
        ret = -EIO;
        goto __exit;
    }
    k_mutex_unlock(&wifi_api_mutex);
    return 0;

__exit:
    k_mutex_unlock(&wifi_api_mutex);
    return ret;
}

int csk_wifi_sta_disconnect(csk_wifi_result_t *result, k_timeout_t timeout)
{
    if (result == NULL) {
        return -EINVAL;
    }
    int ret = 0;
    k_mutex_lock(&wifi_api_mutex, K_FOREVER);
    const struct device *dev = wifi_get_dev(WIFI_MODE_STA);
    ret = wifi_send_command(dev, WIFI_CMD_STA_DISCONNECT, K_NO_WAIT);
    if (ret != 0)   goto __exit;
    ret = wifi_take_result(result, timeout);
    if (ret != 0)   goto __exit;
    if (*result != CSK_WIFI_SUCCESS) {
        ret = -EIO;
        goto __exit;
    }
    k_mutex_unlock(&wifi_api_mutex);
    return 0;

__exit:
    k_mutex_unlock(&wifi_api_mutex);
    return ret;
}

#endif

#ifdef CONFIG_CSK_WIFI_SOFTAP

int csk_wifi_ap_start(const esp_hosted_softap_config_t *ap_config, csk_wifi_result_t *result, k_timeout_t timeout)
{
    if (ap_config == NULL || result == NULL) {
        return -EINVAL;
    }
    int ret = 0;
    k_mutex_lock(&wifi_api_mutex, K_FOREVER);
    const struct device *dev = wifi_get_dev(WIFI_MODE_AP);
    struct csk_wifi_data *data = dev->data;
    memcpy(&data->ap_config, ap_config, sizeof(esp_hosted_softap_config_t));
    ret = wifi_send_command(dev, WIFI_CMD_AP_START, K_NO_WAIT);
    if (ret != 0)   goto __exit;
    ret = wifi_take_result(result, timeout);
    if (ret != 0)   goto __exit;
    if (*result != CSK_WIFI_SUCCESS) {
        ret = -EIO;
        goto __exit;
    }
    k_mutex_unlock(&wifi_api_mutex);
    return 0;

__exit:
    k_mutex_unlock(&wifi_api_mutex);
    return ret;
}

int csk_wifi_ap_stop(csk_wifi_result_t *result, k_timeout_t timeout)
{
    if (result == NULL) {
        return -EINVAL;
    }
    int ret = 0;
    k_mutex_lock(&wifi_api_mutex, K_FOREVER);
    const struct device *dev = wifi_get_dev(WIFI_MODE_AP);
    struct csk_wifi_data *data = dev->data;
    ret = wifi_send_command(dev, WIFI_CMD_AP_STOP, K_NO_WAIT);
    if (ret != 0)   goto __exit;
    ret = wifi_take_result(result, timeout);
    if (ret != 0)   goto __exit;
    if (*result != CSK_WIFI_SUCCESS) {
        ret = -EIO;
        goto __exit;
    }
    return 0;

__exit:
    k_mutex_unlock(&wifi_api_mutex);
    return ret;
}

#endif
