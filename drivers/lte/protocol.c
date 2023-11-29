#include "protocol.h"

#include <stdio.h>

void *find_header(uint8_t *buf, uint16_t len)
{
    uint8_t *ptr = buf;
    uint16_t remind_len = len;
    while (remind_len >= FRAME_HEADER_SIZE) {
        struct csk_payload_header *payload_header = (struct csk_payload_header *)ptr;
        if (le16toh(payload_header->tag) == CSK_HEADER_TAG) {
            int hdr_checksum = payload_header->hdr_checksum;
            int tot_checksum = payload_header->tot_checksum;
            payload_header->hdr_checksum = 0;
            payload_header->tot_checksum = 0;
            int header_checksum = le16toh(compute_checksum((uint8_t *)payload_header, FRAME_HEADER_SIZE));
            payload_header->tot_checksum = tot_checksum;
            payload_header->hdr_checksum = hdr_checksum;
            if (hdr_checksum == header_checksum) {
                return ptr;
            } else {
                // printk("------------- check header sum failed\n");
                ptr++;
                remind_len--;
                continue;
            }
        } else {
            // printk("---------- no tag\n");
            ptr++;
            remind_len--;
            continue;
        }
    }
    return NULL;
}
