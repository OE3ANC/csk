#include "ec718_conn.h"

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#include "protocol_phone_host.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(protocol_phone, LOG_LEVEL_INF);

static sys_slist_t phone_callback_list;

#define MAX_DEFAULT_TINEOUT_MS      1000

#define MAX_PHONE_QUEUE_LEN        5
static struct k_msgq phone_cmd_msgq[PROTO_PHONE_CMD_MAX];
static char phone_cmd_msgq_buffer[PROTO_PHONE_CMD_MAX][sizeof(struct proto_phone_header *) * MAX_PHONE_QUEUE_LEN];

#define PHONE_HEADER_ALLOC(command, args_size) ({ \
        int frame_size = sizeof(struct proto_phone_header) + args_size; \
        struct proto_phone_header *ptr = k_malloc(frame_size); \
            if(!ptr) { \
               LOG_ERR("no mem"); \
            } else { \
                ptr->ret = 0; \
                ptr->cmd = (command); \
                ptr->flags = PROTO_PHONE_FLAG_OWNER_HOST; \
            } \
        ptr; \
    })

#define PHONE_HEADER_FREE(sock_header) do { \
        k_free(sock_header); \
    } while (0)

static inline int proto_phone_send_frame(struct proto_phone_header *phone_header, uint16_t args_size)
{
    return ec718_uart_send_frame_with_ack_v2(PHONE_IF, (uint8_t *)phone_header,
                                            PROTO_PHONE_HEADER_SIZE + args_size);
}

static void phone_dispatch_callbacks(proto_phone_evt_e events, void *event_data, uint32_t event_data_len)
{
    sys_slist_t *list = &phone_callback_list;
    proto_phone_event_cb_t *cb, *tmp;
    SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
        if (cb->events & events) {
            cb->handler(cb->events & events, event_data, event_data_len, cb->arg);
        }
    }
}

int proto_phone_add_callback(proto_phone_event_cb_t *event_cb)
{
    sys_slist_prepend(&phone_callback_list, &event_cb->node);
    return 0;
}

int proto_phone_rx_process(void *phone_frame, int frame_size)
{
    struct proto_phone_header *phone_header = phone_frame;
    if (phone_header->flags & PROTO_PHONE_FLAG_OWNER_HOST) {
        struct proto_phone_header *new_header = k_malloc(frame_size);
        if (!new_header) {
            return -ENOMEM;
        }
        memcpy(new_header, phone_header, frame_size);
        k_msgq_put(&phone_cmd_msgq[new_header->cmd], &new_header, K_FOREVER);
    } else if (phone_header->flags & PROTO_PHONE_FLAG_OWNER_SLAVE) {
        //dispatch to user
        if (phone_header->evt & PROTO_PHONE_REMOTE_CALL) {
            phone_dispatch_callbacks(phone_header->evt, NULL, 0);
        } else if (phone_header->evt & PROTO_PHONE_REMOTE_HANGUP) {
            phone_dispatch_callbacks(phone_header->evt, NULL, 0);
        } else if (phone_header->evt & PROTO_PHONE_REMOTE_CALLING_NUMBER) {
            struct proto_phone_call_number_evt_args *call_args =
                (struct proto_phone_call_number_evt_args *)phone_header->args;
            phone_dispatch_callbacks(phone_header->evt, call_args->number, call_args->number_size);
        } else if (phone_header->evt & PROTO_PHONE_REMOTE_CONNECTED_NUMBER) {
            struct proto_phone_conn_number_evt_args *conn_args =
                (struct proto_phone_conn_number_evt_args *)phone_header->args;
            phone_dispatch_callbacks(phone_header->evt, conn_args->number, conn_args->number_size);
        }
    }
    return 0;
}

static int proto_phone_wait_rsp(struct proto_phone_header *in_frame, void **out_frame, k_timeout_t timeout)
{
    struct proto_phone_header *phone_frame = NULL;
    int ret = k_msgq_get(&phone_cmd_msgq[in_frame->cmd], &phone_frame, timeout);
    if ((ret != 0) || (phone_frame == NULL)) {
        return ret;
    }

    if (out_frame != NULL) {
        *out_frame = phone_frame;
    } else {
        k_free(phone_frame);
    }
    return phone_frame->ret;
}

int proto_phone_call(char *number)
{
    int ret = 0;
    int number_size = strlen(number);
    const int args_size = sizeof(struct proto_phone_call_cmd_args) + number_size;
    struct proto_phone_header *phone_header = PHONE_HEADER_ALLOC(PROTO_PHONE_CALL, args_size);
    if (phone_header == NULL) {
        return -ENOMEM;
    }

    struct proto_phone_call_cmd_args *call_args = (struct proto_phone_call_cmd_args *)phone_header->args;
    call_args->number_size = number_size;
    memcpy(call_args->number, number, number_size);

    proto_phone_send_frame(phone_header, args_size);
    ret = proto_phone_wait_rsp(phone_header, NULL, K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    PHONE_HEADER_FREE(phone_header);

    return ret;
}

int proto_phone_answer(void)
{
    int ret = 0;
    struct proto_phone_header *phone_header = PHONE_HEADER_ALLOC(PROTO_PHONE_ANSWER, 0);
    if (phone_header == NULL) {
        return -ENOMEM;
    }

    proto_phone_send_frame(phone_header, 0);
    ret = proto_phone_wait_rsp(phone_header, NULL, K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    PHONE_HEADER_FREE(phone_header);

    return ret;
}

int proto_phone_hangup(void)
{
    int ret = 0;
    struct proto_phone_header *phone_header = PHONE_HEADER_ALLOC(PROTO_PHONE_HANGUP, 0);
    if (phone_header == NULL) {
        return -ENOMEM;
    }

    proto_phone_send_frame(phone_header, 0);
    ret = proto_phone_wait_rsp(phone_header, NULL, K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    PHONE_HEADER_FREE(phone_header);

    return ret;
}

int proto_phone_init(void)
{
    sys_slist_init(&phone_callback_list);
    for (int i = 0; i < PROTO_PHONE_CMD_MAX; i++) {
        k_msgq_init(&phone_cmd_msgq[i], &phone_cmd_msgq_buffer[i][0],
                    sizeof(struct proto_phone_header *), MAX_PHONE_QUEUE_LEN);
    }
    return 0;
}
