
#include <zephyr/kernel.h>
#include <string.h>
#include <stdlib.h>

#include "gcl_webusb_listenai_protocol.h"
#include "gcl_webusb.h"

int webusb_gcl_send(uint8_t *data, uint32_t data_len, bitmap_action_e action) {
  struct usb_cfg_data usb_cfg = get_usb_cfg();
  uint8_t ep = usb_cfg.endpoint[CONFIG_WEBUSB_IN_EP_IDX].ep_addr;

  bitmap_header_t hdr = create_header(action, data_len);
  uint8_t *header = (uint8_t *)(&hdr);

  int ret;

  ret = usb_transfer_sync(ep, header, BITMAP_HEADER_LEN, USB_TRANS_WRITE);

  if (ret < 0) {
    return ret;
  }

  ret = usb_transfer_sync(ep, data, data_len, USB_TRANS_WRITE);

  return ret;
}

int webusb_gcl_send_bitmap(uint8_t *data, uint32_t data_len)
{
	int ret = webusb_gcl_send(data, data_len, e_action_bitmap);

	return ret;
}

int webusb_gcl_send_shoulder(uint8_t *data, uint32_t data_len)
{
	int ret = webusb_gcl_send(data, data_len, e_action_shoulder);

	return ret;
}

int webusb_gcl_send_gesture(uint8_t *data, uint32_t data_len)
{
	int ret = webusb_gcl_send(data, data_len, e_action_gesture);

	return ret;
}

int webusb_gcl_send_face(face_t face, char *text)
{
	bool isTextEmpty = text == NULL || strlen(text) == 0;

	uint32_t len = isTextEmpty ? sizeof(face) : sizeof(face) + strlen(text);
	uint8_t *data = malloc(len);

	memcpy((void *)data, (void *)&face, len);

	if (!isTextEmpty) {
		uint32_t index = sizeof(face);
		memcpy(data + index, text, strlen(text));
	}

	int ret = webusb_gcl_send(data, len, e_action_face);

	free(data);
	return ret;
}

int webusb_gcl_send_message(char *text)
{
	int ret = webusb_gcl_send((uint8_t *)text, strlen(text), e_action_msg);
	return ret;
}

int webusb_gcl_send_image_info(img_info_t img_info)
{
	uint32_t len = sizeof(img_info);

	uint8_t *data = (uint8_t *)(&img_info);

	int ret = webusb_gcl_send(data, len, e_action_image_info);

	return ret;
}

int webusb_gcl_register_directive(directive_t directive)
{
	uint32_t text_len = strlen(directive.text);
	uint32_t len = sizeof(directive.event) + text_len;
	uint8_t *data = malloc(len);

	if (data == NULL) {
		return 0;
	}

	memcpy(data, &directive.event, sizeof(directive.event));

	uint32_t index = sizeof(directive.event);
	memcpy(data + index, directive.text, text_len);

	int ret = webusb_gcl_send(data, len, e_action_directive);

	free(data);

	return ret;
}
