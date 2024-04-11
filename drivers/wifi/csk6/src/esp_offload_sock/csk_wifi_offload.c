#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/socket_offload.h>

#include "csk6/priv/csk_wifi_priv.h"
#include "esp_offload_socket.h"
#include "ec718_conn.h"
#include "protocol_wifi_host.h"

struct ec718_drv_data {
	/*
	 * Network interface of the sim module.
	 */
	struct net_if *netif;
    uint8_t mac_addr[6];
};

static struct ec718_drv_data s_ec718_drv_data;

int csk_wifi_init(void)
{
    esp_conn_init();
    return 0;
}

static int dummy_get(sa_family_t family,
				enum net_sock_type type,
				enum net_ip_protocol ip_proto,
				struct net_context **context)
{
	// LOG_ERR("NET_SOCKETS_OFFLOAD must be configured for this driver");
	return -1;
}

static struct net_offload offload_funcs = {
	.get = dummy_get,
	.bind = NULL,
	.listen = NULL,
	.connect = NULL,
	.accept = NULL,
	.send = NULL,
	.sendto = NULL,
	.recv = NULL,
	.put = NULL,
};

static void net_iface_init(struct net_if *iface)
{
    const struct device *dev = net_if_get_device(iface);
	struct ec718_drv_data *data = dev->data;

    // esp_conn_init();

    // get wifi mac address


    net_if_set_link_addr(iface, data->mac_addr, sizeof(data->mac_addr), NET_LINK_ETHERNET);
    data->netif = iface;
    iface->if_dev->offload = &offload_funcs;
	socket_offload_dns_register(&esp_offload_dns_ops);
}

// static struct offloaded_if_api api_funcs = {
// 	.iface_api.init = net_iface_init,
// };

static int esp_mgmt_scan(const struct device *dev, scan_result_cb_t cb)
{
	int status = 0;

	return status;
}

static int esp_mgmt_connect(const struct device *dev,
				   struct wifi_connect_req_params *params)
{
	int ret = 0;

	struct proto_wifi_conn_cmd_args conn_arg;
	strcpy(conn_arg.ssid, params->ssid);
	strcpy(conn_arg.pwd, params->psk);

	ret = proto_wifi_sta_connect(&conn_arg);
	printk("wifi esp_mgmt_connect ret: %d\n", ret);

	struct ec718_drv_data *data = dev->data;
	wifi_mgmt_raise_connect_result_event(data->netif,
						     ret);

	return ret ? -EIO : ret;
}

static int esp_mgmt_disconnect(const struct device *dev)
{
	int ret = 0;

	return ret ? -EIO : ret;
}

static const struct net_wifi_mgmt_offload api_funcs = {
	.wifi_iface.iface_api.init = net_iface_init,
	.scan			        = esp_mgmt_scan,
	.connect		        = esp_mgmt_connect,
	.disconnect		        = esp_mgmt_disconnect,
    .ap_enable              = NULL,
    .ap_disable             = NULL,
    .iface_status           = NULL,
#ifdef CONFIG_NET_STATISTICS_WIFI
	.get_stats              = NULL,
#endif /* CONFIG_NET_STATISTICS_WIFI */
    .set_power_save         = NULL,
    .set_twt                = NULL,
    .get_power_save_config  = NULL,
    .reg_domain             = NULL,
};

static int ec718_init(const struct device *dev)
{
    // printk("-------------- %s\n", __func__);
    return 0;
}

NET_DEVICE_OFFLOAD_INIT(ec718_dev, "ec718_net", ec718_init, NULL, &s_ec718_drv_data,
                        NULL, 80, &api_funcs, 1472);
