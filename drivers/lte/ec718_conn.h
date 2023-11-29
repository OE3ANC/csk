#pragma once


#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t if_type;
	uint8_t flag;
	uint16_t payload_len;
	void *frame;
	void (*free_buf_handle)(void *buf_handle);
} ec718_buffer_handle_t;

int ec718_conn_init(void);
int ec718_uart_write(uint8_t *data, uint16_t size);
int ec718_uart_send_frame(csk_interface_type_e iface, uint8_t *buf, uint16_t len);
int ec718_uart_send_frame_with_ack_v2(csk_interface_type_e iface, uint8_t *buf, uint16_t len);
// int ec718_uart_send_frame_v2(csk_payload_header_t *frame);
// int ec718_uart_send_frame_v2(csk_interface_type_e iface, uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif
