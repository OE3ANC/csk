#pragma once

#include "protocol_phone.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*phone_event_handler_t) (proto_phone_evt_e event, void *event_data, uint32_t event_data_len, void *arg);

typedef struct proto_phone_event_cb {
    /**
     * This is meant to be used in the driver core and the user should not
	 * touch it (see csk_wifi_api.c).
	 */
    sys_snode_t node;

    /**
     * This function will be called while WIFI event occurred
     */
    phone_event_handler_t handler;

    /**
     * Wifi events mask, see `csk_wifi_event_t`
     */
    uint32_t events;

    /**
     * Wifi event data
     */
    void *event_data;

    /**
     * Wifi event data len
     */
    uint32_t event_data_len;

    /**
     * user argument
     */
    void *arg;
} proto_phone_event_cb_t;

int proto_phone_call(char *number);
int proto_phone_answer(void);
int proto_phone_hangup(void);

int proto_phone_add_callback(proto_phone_event_cb_t *event_cb);

#ifdef __cplusplus
}
#endif
