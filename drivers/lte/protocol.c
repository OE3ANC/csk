#include "protocol.h"

#include <stdio.h>

bool is_valid_tag(uint8_t *buf)
{
    struct csk_payload_header *payload_header = (struct csk_payload_header *)buf;
    if (le16toh(payload_header->tag) == CSK_HEADER_TAG) {
        return true;
    } else {
        return false;
    }
}

bool is_valid_hdr_checksum(uint8_t *buf)
{
    struct csk_payload_header *payload_header = (struct csk_payload_header *)buf;
    int hdr_checksum = payload_header->hdr_checksum;
    int tot_checksum = payload_header->tot_checksum;
    payload_header->hdr_checksum = 0;
    payload_header->tot_checksum = 0;
    int header_checksum = le16toh(compute_checksum(buf, FRAME_HEADER_SIZE));
    payload_header->tot_checksum = tot_checksum;
    payload_header->hdr_checksum = hdr_checksum;
    if (hdr_checksum == header_checksum) {
        return true;
    } else {
        return false;
    }
}

bool is_valid_tot_checksum(uint8_t *buf)
{
    struct csk_payload_header *payload_header = (struct csk_payload_header *)buf;
    int hdr_checksum = payload_header->hdr_checksum;
    int tot_checksum = payload_header->tot_checksum;
    payload_header->tot_checksum = 0;
    int total_checksum = le16toh(compute_checksum(buf, FRAME_HEADER_SIZE + payload_header->payload_len));
    payload_header->tot_checksum = tot_checksum;
    payload_header->hdr_checksum = hdr_checksum;
    if (total_checksum == tot_checksum) {
        return true;
    } else {
        return false;
    }
}
