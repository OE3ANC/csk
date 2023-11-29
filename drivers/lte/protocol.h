#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_HEADER_SIZE                       sizeof(struct csk_payload_header)
#define MAX_FRAME_PAYLOAD_SIZE                  (2048)
#define MAX_FRAME_SIZE                          (MAX_FRAME_PAYLOAD_SIZE + FRAME_HEADER_SIZE)

#define htole16(x)                        ((uint16_t)(x))   //for write
#define le16toh(x)                        ((uint16_t)(x))   //for read

#define CSK_HEADER_TAG                    0xa5a5
#define CSK_HEADER_FLAG_ACK           	  BIT(0)
#define CSK_HEADER_FLAG_ACK_REQ           BIT(1)

struct csk_payload_header {
    uint16_t         tag;
	uint8_t          if_type;
	uint8_t          flags;
	uint16_t         payload_len;
	uint16_t         tot_checksum;
	uint16_t         hdr_checksum;
	uint16_t		 seq_num;
	uint16_t         reserved;
	uint8_t			 payload[0];
} __attribute__((packed));

typedef enum {
    AT_CMD_IF,
    SOCKET_IF,
	CSK_IFACE_MAX,
} csk_interface_type_e;

static inline uint16_t compute_checksum(uint8_t *buf, uint16_t len)
{
	uint16_t checksum = 0;
	uint16_t i = 0;

	while(i < len) {
		checksum += buf[i];
		i++;
	}

	return checksum;
}

void *find_header(uint8_t *buf, uint16_t len);

#ifdef __cplusplus
}
#endif
