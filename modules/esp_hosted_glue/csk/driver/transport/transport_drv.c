// SPDX-License-Identifier: Apache-2.0
// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/** Includes **/
#include "transport_drv.h"
#include "stats.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(transport_drv, CONFIG_ESP_HOSTED_LOG_LEVEL);

/**
 * @brief  Slave capabilities are parsed
 *         Currently no added functionality to that
 * @param  None
 * @retval None
 */
static char chip_type = ESP_PRIV_FIRMWARE_CHIP_UNRECOGNIZED;
static struct fw_version esp_fw_version= {0};

/**
 * @brief  esp_get_version
 * @param  
 * @retval esp version
 */
uint8_t esp_get_version(void)
{
	return esp_fw_version.major1 * 10 + esp_fw_version.minor;
}

/**
 * @brief  open virtual network device
 * @param  netdev - network device
 * @retval 0 on success
 */
int esp_netdev_open(netdev_handle_t netdev)
{
	return STM_OK;
}

/**
 * @brief  close virtual network device
 * @param  netdev - network device
 * @retval 0 on success
 */
int esp_netdev_close(netdev_handle_t netdev)
{
	return STM_OK;
}


/**
 * @brief  transmit on virtual network device
 * @param  netdev - network device
 *         net_buf - buffer to transmit
 * @retval STM_OK for success or failure from enum stm_ret_t
 */
int esp_netdev_xmit(netdev_handle_t netdev, struct net_pbuf *net_buf)
{
	struct esp_private *priv = NULL;
	int ret = 0;

	if (!netdev || !net_buf)
		return STM_FAIL;
	priv = (struct esp_private *) netdev_get_priv(netdev);

	if (!priv)
		return STM_FAIL;

	ret = send_to_slave(priv->if_type, priv->if_num,
			net_buf->payload, net_buf->len);
	hosted_free(net_buf);

	return ret;
}

void process_capabilities(uint8_t cap)
{
#if DEBUG_TRANSPORT
	LOG_DBG("capabilities: 0x%x",cap);
#else
	/* warning suppress */
	if(cap);
#endif
}

void process_priv_communication(struct net_pbuf *pbuf)
{
	struct esp_priv_event *header = NULL;

	uint8_t *payload = NULL;
	uint16_t len = 0;

	if (!pbuf || !pbuf->payload)
		return;

	header = (struct esp_priv_event *) pbuf->payload;

	payload = pbuf->payload;
	len = pbuf->len;

	if (header->event_type == ESP_PRIV_EVENT_INIT) {
		LOG_DBG("event packet type");
		process_event(payload, len);
	}

	hosted_free(pbuf->payload);
	hosted_free(pbuf);
}

void print_capabilities(uint32_t cap)
{
	LOG_DBG("Features supported are:");
	if (cap & ESP_WLAN_SDIO_SUPPORT)
		LOG_DBG("\t * WLAN");
	if ((cap & ESP_BT_UART_SUPPORT) || (cap & ESP_BT_SDIO_SUPPORT)) {
		LOG_DBG("\t * BT/BLE");
		if (cap & ESP_BT_UART_SUPPORT)
			LOG_DBG("\t   - HCI over UART");
		if (cap & ESP_BT_SDIO_SUPPORT)
			LOG_DBG("\t   - HCI over SDIO");
		if ((cap & ESP_BLE_ONLY_SUPPORT) && (cap & ESP_BR_EDR_ONLY_SUPPORT))
			LOG_DBG("\t   - BT/BLE dual mode");
		else if (cap & ESP_BLE_ONLY_SUPPORT)
			LOG_DBG("\t   - BLE only");
		else if (cap & ESP_BR_EDR_ONLY_SUPPORT)
			LOG_DBG("\t   - BR EDR only");
	}
}

int check_esp_version(struct fw_version *ver)
{
	#define VERSION_BUFFER_SIZE 50
	char version_str[VERSION_BUFFER_SIZE] = { 0 };

	memcpy(&esp_fw_version, ver, sizeof(struct fw_version));

	snprintf(version_str, VERSION_BUFFER_SIZE, "%s-%u.%u.%u.%u.%u",
			ver->project_name, ver->major1, ver->major2, ver->minor, ver->revision_patch_1, ver->revision_patch_2);

	LOG_INF("ESP Firmware version: %s", version_str);
	return 0;
}

int process_fw_data(struct fw_version *fw_p, int tag_len)
{
	if (tag_len != sizeof(struct fw_version)) {
		LOG_ERR("Length not matching to firmware data size");
		return -1;
	}

	return check_esp_version(fw_p);
}

void process_event(uint8_t *evt_buf, uint16_t len)
{
	int ret = 0;
	struct esp_priv_event *event;

	if (!evt_buf || !len)
		return;

	event = (struct esp_priv_event *) evt_buf;

	if (event->event_type == ESP_PRIV_EVENT_INIT) {

		LOG_DBG("Received INIT event from ESP peripheral");

		print_hex_dump(event->event_data, event->event_len, "process event");

		ret = process_init_event(event->event_data, event->event_len);
		if (ret) {
			LOG_ERR("failed to init event");
		}
	} else {
		LOG_ERR("Drop unknown event");
	}
}

int process_init_event(uint8_t *evt_buf, uint8_t len)
{
	uint8_t len_left = len, tag_len;
	uint8_t *pos;
	struct fw_version *fw_p;
	int ret = 0;

	if (!evt_buf)
		return STM_FAIL;
	pos = evt_buf;
	while (len_left) {
		tag_len = *(pos + 1);
		LOG_DBG("EVENT: %d", *pos);
		if (*pos == ESP_PRIV_CAPABILITY) {
			LOG_DBG("priv capabilty ");
			process_capabilities(*(pos + 2));
			print_capabilities(*(pos + 2));
		} else if (*pos == ESP_PRIV_SPI_CLK_MHZ) {
			// adjust spi clock
		} else if (*pos == ESP_PRIV_FIRMWARE_CHIP_ID) {
			chip_type = *(pos+2);
		} else if (*pos == ESP_PRIV_TEST_RAW_TP) {
			LOG_DBG("priv test raw tp");
#if TEST_RAW_TP
			process_test_capabilities(*(pos + 2));
#endif
		} else if (*pos == ESP_PRIV_FW_DATA) {
			fw_p = (struct fw_version *)(pos + 2);
			ret = process_fw_data(fw_p, tag_len);
			if(ret != 0) {
				return ret;
			}
			
		} else {
			LOG_ERR("Unsupported tag in event");
		}
		pos += (tag_len+2);
		len_left -= (tag_len+2);
	}

	if ((chip_type != ESP_PRIV_FIRMWARE_CHIP_ESP32) &&
	    (chip_type != ESP_PRIV_FIRMWARE_CHIP_ESP32S2) &&
	    (chip_type != ESP_PRIV_FIRMWARE_CHIP_ESP32C2) &&
	    (chip_type != ESP_PRIV_FIRMWARE_CHIP_ESP32C3) &&
	    (chip_type != ESP_PRIV_FIRMWARE_CHIP_ESP32C6) &&
	    (chip_type != ESP_PRIV_FIRMWARE_CHIP_ESP32S3)) {
		LOG_ERR("ESP board type is not mentioned, ignoring [%d]", chip_type);
		chip_type = ESP_PRIV_FIRMWARE_CHIP_UNRECOGNIZED;
		return -1;
	} else {
		LOG_DBG("ESP board type is : %d ", chip_type);
	}

	return STM_OK;
}
