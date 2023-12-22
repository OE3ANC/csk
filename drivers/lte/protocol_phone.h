#pragma once

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define PROTO_PHONE_FLAG_OWNER_HOST         BIT(0)
#define PROTO_PHONE_FLAG_OWNER_SLAVE        BIT(1)

#define PROTO_PHONE_HEADER_SIZE  sizeof(struct proto_phone_header)

struct proto_phone_header {
    union {
        uint8_t cmd;
        uint8_t evt;
    };
    uint8_t flags;
    int ret;
    uint8_t args[0];
} __attribute__((packed));

// A --> csk-ec718    B --> remote phone
typedef enum {
    PROTO_PHONE_CALL,    //A call B
    PROTO_PHONE_ANSWER,  //B call A, A answer the call
    PROTO_PHONE_HANGUP,  //B call A, A hang up
    PROTO_PHONE_CMD_MAX,
} proto_phone_cmd_e;

typedef enum {
    PROTO_PHONE_REMOTE_CALL             = BIT(0),    //B call A
    PROTO_PHONE_REMOTE_ANSWER           = BIT(1),   //A call B, B answer the call
    PROTO_PHONE_REMOTE_HANGUP           = BIT(2),  //A call B, B hang up
    PROTO_PHONE_REMOTE_CALLING_NUMBER   = BIT(3),  //calling number
    PROTO_PHONE_REMOTE_CONNECTED_NUMBER = BIT(4),  //connected number
    PROTO_PHONE_EVT_MAX,
} proto_phone_evt_e;

struct proto_phone_call_cmd_args {
    uint8_t number_size;
    uint8_t number[0];
} __attribute__((packed));

struct proto_phone_call_number_evt_args {
    uint8_t number_size;
    uint8_t number[0];
} __attribute__((packed));

struct proto_phone_conn_number_evt_args {
    uint8_t number_size;
    uint8_t number[0];
} __attribute__((packed));

int proto_phone_rx_process(void *phone_frame, int frame_size);
int proto_phone_init(void);

#ifdef __cplusplus
}
#endif
