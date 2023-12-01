#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include "ec718_conn.h"

#include "protocol_socket_host.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ec718_conn, LOG_LEVEL_INF);


#define MAX_RX_FRAME_NUMS                       (10)
#define MAX_RX_LITTLE_FRAME_NUMS                (10)

#define MAX_RETRANCATION_TIMES                  3
#define MAX_ACK_WAIT_TIME_MS                    500

#define MAX_RECV_LEN        MAX_FRAME_SIZE

// #define MAX_BIG_FRAME_SIZE                      MAX_FRAME_SIZE
// #define MAX_BIG_FRAME_NUMS                       (10)
// #define MAX_MIDDLE_FRAME_SIZE                   (100 + FRAME_HEADER_SIZE)
// #define MAX_MIDDLE_FRAME_NUMS                    (20)
// #define MAX_LITTLE_FRAME_SIZE                   FRAME_HEADER_SIZE
// #define MAX_LITTLE_FRAME_NUMS                    (10)

K_PIPE_DEFINE(s_rx_pipe, MAX_RECV_LEN * 2, 4);
K_SEM_DEFINE(s_uart_rx_sem, 0, 1);

typedef struct {
	void *fifo_reserved;
	uint8_t *data;
	uint32_t size;
    void (*free_frame)(void *frame);
} frame_fifo_item_t;

typedef struct {
    const struct device *dev;
    struct k_fifo big_frame_fifo;
    frame_fifo_item_t frame[MAX_RX_FRAME_NUMS];
    struct k_fifo little_frame_fifo;
    frame_fifo_item_t little_frame[MAX_RX_LITTLE_FRAME_NUMS];
    frame_fifo_item_t *current_frame;
    int rx_state;
    struct k_msgq ack_msgq[CSK_IFACE_MAX];
} ec718_handle_t;

K_MSGQ_DEFINE(frame_msgq, sizeof(frame_fifo_item_t *), 10, 4);
K_MSGQ_DEFINE(sock_frame_msgq, sizeof(frame_fifo_item_t *), 10, 4);
static uint16_t s_tx_seq_num = 1;
static uint16_t s_rx_seq_num = 1;
// K_SEM_DEFINE(ack_sem, 0, 1);

K_MSGQ_DEFINE(ack_seq_cache, sizeof(uint32_t), 10, 4);

static char ack_msgq_buffer[CSK_IFACE_MAX][sizeof(uint32_t) * 10];

// struct k_mem_slab big_slab;
// struct k_mem_slab middle_slab;
// struct k_mem_slab little_slab;
// static char __aligned(4) big_slab_buffer[MAX_BIG_FRAME_SIZE * MAX_BIG_FRAME_NUMS];
// static char __aligned(4) middle_slab_buffer[MAX_MIDDLE_FRAME_SIZE * MAX_MIDDLE_FRAME_NUMS];
// static char __aligned(4) little_slab_buffer[MAX_LITTLE_FRAME_SIZE * MAX_LITTLE_FRAME_NUMS];

static void ec718_uart_send_ack(csk_interface_type_e iface, uint16_t seq_num);

static ec718_handle_t s_ec718_handle = {
    .dev = DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(lte))),
};

static int big_cnt = 0;
static int lit_cnt = 0;

void free_big_frame(void *frame)
{
    big_cnt--;
    frame_fifo_item_t *in_frame = frame;
    in_frame->size = 0;
    memset(in_frame->data, 0, FRAME_HEADER_SIZE);
    k_fifo_put(&s_ec718_handle.big_frame_fifo, in_frame);
}

void free_little_frame(void *frame)
{
    lit_cnt--;
    frame_fifo_item_t *in_frame = frame;
    in_frame->size = 0;
    memset(in_frame->data, 0, FRAME_HEADER_SIZE);
    k_fifo_put(&s_ec718_handle.little_frame_fifo, in_frame);
}

frame_fifo_item_t *alloca_frame_item(uint16_t payload_size)
{
    frame_fifo_item_t *frame_item = NULL;
    if (payload_size == 0) {
        lit_cnt++;
        frame_item = k_fifo_get(&s_ec718_handle.little_frame_fifo, K_NO_WAIT);
        if (!frame_item) {
            big_cnt++;
            frame_item = k_fifo_get(&s_ec718_handle.big_frame_fifo, K_NO_WAIT);
        }
    } else {
        big_cnt++;
        frame_item = k_fifo_get(&s_ec718_handle.big_frame_fifo, K_NO_WAIT);
    }
    return frame_item;
}

static void payload_checksum_failed_handle(uint8_t *payload, uint16_t size, frame_fifo_item_t *current_frame)
{
    uint8_t *ptr = payload;
    int remind_size = size;
    while (remind_size >= FRAME_HEADER_SIZE) {
        struct csk_payload_header *next_header = (struct csk_payload_header *)find_header(ptr, remind_size);
        if (next_header == NULL) {
            memmove(current_frame->data, &current_frame->data[current_frame->size - (FRAME_HEADER_SIZE - 1)], FRAME_HEADER_SIZE - 1);
            current_frame->size = FRAME_HEADER_SIZE - 1;
            break;
        }
        int diff = (uint8_t *)next_header - ptr;
        remind_size -= diff;
        ptr = (uint8_t *)next_header;
        if (remind_size < (next_header->payload_len + FRAME_HEADER_SIZE)) {
            current_frame->size = 0;
            break;  //Will lose valid data ?
        }

        int total_checksum = compute_checksum((uint8_t *)next_header, FRAME_HEADER_SIZE + next_header->payload_len);
        if (total_checksum != next_header->tot_checksum) {
            ptr += FRAME_HEADER_SIZE;
            remind_size -= FRAME_HEADER_SIZE;
            continue;  //find next valid header
        }

        frame_fifo_item_t *send_frame = alloca_frame_item(next_header->payload_len);
        if (!send_frame) {
            break;  //no mem, throw all
        }
        memcpy(send_frame->data, next_header, next_header->payload_len + FRAME_HEADER_SIZE);
        send_frame->size = next_header->payload_len + FRAME_HEADER_SIZE;
        int ret = k_msgq_put(&frame_msgq, &send_frame, K_NO_WAIT);  //send to process
        if (ret != 0) {
            printk("-----msgq full, data lose\n");
        }
        remind_size -= send_frame->size;
        ptr += send_frame->size;
    }
}

static void uart_rx_callback(const struct device *dev, void *user_data)
{
    uint8_t temp_buf[64];
	uart_irq_update(dev);
	while (uart_irq_is_pending(dev)) {
		if (uart_irq_rx_ready(dev)) {
			int recv_len = uart_fifo_read(dev, temp_buf, sizeof(temp_buf));
            int bytes_written = 0;
			int ret = k_pipe_put(&s_rx_pipe, temp_buf, recv_len, &bytes_written, 1, K_NO_WAIT);
            if (ret != 0) {
                printk("fifo is full\n");
            }
            k_sem_give(&s_uart_rx_sem);
		}
	}
}

static void uart_rx_process(void *arg1, void *arg2, void *arg3)
{
    int ret = 0;
    while (1) {
        k_sem_take(&s_uart_rx_sem, K_FOREVER);
        while (k_pipe_read_avail(&s_rx_pipe) > 0) {
            frame_fifo_item_t *frame = s_ec718_handle.current_frame;
            if (!frame) {
                frame = alloca_frame_item(MAX_FRAME_PAYLOAD_SIZE);
                if (!frame) {
                    // printk("no mem, data lose\n");
                    break;
                }
                s_ec718_handle.current_frame = frame;
            }

            if (s_ec718_handle.rx_state == 0) {  //rx header
                    // int recv_len = uart_fifo_read(dev, &frame->data[frame->size], MAX_FRAME_SIZE - frame->size);
                    int bytes_read = 0;
                    ret = k_pipe_get(&s_rx_pipe, &frame->data[frame->size], FRAME_HEADER_SIZE - frame->size, &bytes_read, 1, K_NO_WAIT);
                    // if (ret != 0) {
                    //     continue;
                    // }
                    int recv_len = bytes_read;
                    // int recv_len = ring_buf_read(s_ec718_handle.ringbuf, &frame->data[frame->size], MAX_FRAME_SIZE - frame->size);
                    frame->size += recv_len;
                    if (frame->size >= FRAME_HEADER_SIZE) {
                        uint8_t *header = find_header(frame->data, frame->size);
                        if (header != NULL) {
                            if (header != frame->data) {
                                int diff = header - frame->data;
                                frame->size -= diff;
                                printk("-------------------throw garbage data: %d\n", diff);
                                memmove(frame->data, header, frame->size);
                            }
                            struct csk_payload_header *payload_header = (struct csk_payload_header *)header;
                            if (payload_header->payload_len == 0) {  //frame without payload
                                frame_fifo_item_t *no_payload_frame = alloca_frame_item(0);
                                if (!no_payload_frame) {
                                    // printk("no mem, data lose\n");
                                    frame->size = 0;
                                    s_ec718_handle.rx_state = 0;
                                    break;
                                }
                                memcpy(no_payload_frame->data, frame->data, FRAME_HEADER_SIZE);
                                no_payload_frame->size = FRAME_HEADER_SIZE;

                                ret = k_msgq_put(&frame_msgq, &no_payload_frame, K_NO_WAIT);
                                if (ret != 0) {
                                    printk("msgq is full, data loss\n");
                                }
                                frame->size = 0;
                                s_ec718_handle.rx_state = 0;
                                // continue;
                            } else {
                                s_ec718_handle.rx_state = 1;
                            }
                            // ECCUST_PRINTF(UNILOG_LISTENAI_CONN, __LINE__, P_INFO,"got frame header");
                        } else {
                            memmove(frame->data, &frame->data[frame->size -1 - (FRAME_HEADER_SIZE - 1)], FRAME_HEADER_SIZE - 1);
                            frame->size = FRAME_HEADER_SIZE - 1;
                            s_ec718_handle.rx_state = 0;
                            printk("---------------------------- no header\n");
                        }
                    }
                }
                if (s_ec718_handle.rx_state == 1) {  //rx payload
                    struct csk_payload_header *payload_header = (struct csk_payload_header *)frame->data;
                    int to_read = payload_header->payload_len - (frame->size - FRAME_HEADER_SIZE);

                    int bytes_read = 0;
                    ret = k_pipe_get(&s_rx_pipe, &frame->data[frame->size], to_read, &bytes_read, 1, K_NO_WAIT);
                    // if (ret != 0) {
                    //     continue;
                    // }
                    int recv_len = bytes_read;
                    // int to_read = payload_header->payload_len - (frame->size - FRAME_HEADER_SIZE);
                    // int recv_len = ring_buf_read(s_ec718_handle.ringbuf, &frame->data[frame->size], to_read);
                    frame->size += recv_len;

                    if (frame->size >= (payload_header->payload_len + FRAME_HEADER_SIZE)) {
                        int tot_checksum = le16toh(payload_header->tot_checksum);
                        payload_header->tot_checksum = 0;
                        int total_checksum = compute_checksum(frame->data, FRAME_HEADER_SIZE + payload_header->payload_len);
                        payload_header->tot_checksum = tot_checksum;
                        if (total_checksum == tot_checksum) {
                            // ECCUST_PRINTF(UNILOG_LISTENAI_CONN, __LINE__, P_INFO,"got frame payload");

                            ret = k_msgq_put(&frame_msgq, &frame, K_NO_WAIT);
                            if (ret != 0) {
                                printk("-----msgq full, data lose\n");
                            }
                            // os_ret = xQueueSend(frame_queue, &frame, 0);
                            // if (os_ret != pdPASS) {
                            //     // printk("-----msgq full, data lose\n");
                            // }
                            s_ec718_handle.rx_state = 0;
                            s_ec718_handle.current_frame = NULL;
                        } else {
                            //payload checksum failed, lose payload
                            // ECCUST_DUMP(UNILOG_LISTENAI_CONN, __LINE__, P_INFO, "dump: ", frame->size, frame->data);
                            printk("payload checksum failed, %d, calsum: %d  prosum:%d", frame->size, total_checksum, tot_checksum);
                            payload_checksum_failed_handle(&frame->data[FRAME_HEADER_SIZE], payload_header->payload_len, frame);
                            s_ec718_handle.rx_state = 0;
                        }
                    }
                }
        }
    }
}

static void rx_process(void *arg1, void *arg2, void *arg3)
{
    int ret = 0;
    frame_fifo_item_t *frame = NULL;
    uint32_t ack_seq_num = 0;
    while (1) {
        ret = k_msgq_get(&frame_msgq, &frame, K_FOREVER);
        if (ret != 0 || frame == NULL) {
            continue;
        }

        struct csk_payload_header *payload_header = (struct csk_payload_header *)frame->data;

        if (payload_header->flags & CSK_HEADER_FLAG_ACK_REQ) {
            ec718_uart_send_ack(payload_header->if_type, payload_header->seq_num);
        } else if (payload_header->flags & CSK_HEADER_FLAG_ACK) {
            ack_seq_num = le16toh(payload_header->seq_num);
            k_msgq_put(&s_ec718_handle.ack_msgq[payload_header->if_type], &ack_seq_num, K_FOREVER);
            if (k_msgq_num_free_get(&ack_seq_cache) > 0) {
                k_msgq_put(&ack_seq_cache, &ack_seq_num, K_FOREVER);
            } else {
                uint32_t temp;
                k_msgq_get(&ack_seq_cache, &temp, K_FOREVER);
                k_msgq_put(&ack_seq_cache, &ack_seq_num, K_FOREVER);
            }
            goto __cleanup;
        } else {
            LOG_HEXDUMP_ERR(frame->data, sizeof(struct csk_payload_header ), "err header");
            printk("-------- unknown package\n");
        }


        if (le16toh(payload_header->seq_num) == s_rx_seq_num) {
            s_rx_seq_num = le16toh(payload_header->seq_num) + 1;
            // if (s_rx_seq_num >= 2000) {
            //     printk("yes recv_seq:%d  current_rx_seq:%d  tx: %d\n",
            //         le16toh(payload_header->seq_num), s_rx_seq_num, s_tx_seq_num);
            // }
        } else {
            printk("package out of date, maybe lost ack, recv_seq:%d  current_rx_seq:%d  tx: %d\n",
                    le16toh(payload_header->seq_num), s_rx_seq_num, s_tx_seq_num);
            if (frame->free_frame) {
                frame->free_frame(frame);
            }
            continue;
        }

        if (payload_header->if_type == AT_CMD_IF) {
        } else if (payload_header->if_type == SOCKET_IF) {
            k_msgq_put(&sock_frame_msgq, &frame, K_FOREVER);
            k_yield();
            continue;
        }

__cleanup:
        if (frame->free_frame) {
            frame->free_frame(frame);
        }
    }
}

static void rx_socket_process(void *arg1, void *arg2, void *arg3)
{
    int ret = 0;
    frame_fifo_item_t *frame = NULL;
    while (1) {
        ret = k_msgq_get(&sock_frame_msgq, &frame, K_FOREVER);
        if (ret != 0 || frame == NULL) {
            continue;
        }
        struct csk_payload_header *payload_header = (struct csk_payload_header *)frame->data;
        proto_sock_rx_process_v2(frame->data, frame->size);
        if (frame->free_frame) {
            frame->free_frame(frame);
        }
    }
}

int ec718_conn_init(void)
{
    if (!device_is_ready(s_ec718_handle.dev)) {
        printk("---------------- not ready\n");
        return -1;
    }

    s_ec718_handle.rx_state = 0;
    k_fifo_init(&s_ec718_handle.big_frame_fifo);
    k_fifo_init(&s_ec718_handle.little_frame_fifo);
    for (int i = 0; i < MAX_RX_FRAME_NUMS; i++) {
        s_ec718_handle.frame[i].data = k_malloc(MAX_FRAME_SIZE);
        if (!s_ec718_handle.frame[i].data) {
            printk("----------- no mem\n");
            break;
        }
        s_ec718_handle.frame[i].size = 0;
        s_ec718_handle.frame[i].free_frame = free_big_frame;
        k_fifo_put(&s_ec718_handle.big_frame_fifo, &s_ec718_handle.frame[i]);
    }

    for (int i = 0; i < MAX_RX_LITTLE_FRAME_NUMS; i++) {
        s_ec718_handle.little_frame[i].data = k_malloc(FRAME_HEADER_SIZE);
        if (!s_ec718_handle.little_frame[i].data) {
            printk("----------- no mem\n");
            break;
        }
        s_ec718_handle.little_frame[i].size = 0;
        s_ec718_handle.little_frame[i].free_frame = free_little_frame;
        k_fifo_put(&s_ec718_handle.little_frame_fifo, &s_ec718_handle.little_frame[i]);
    }

    for (int i = 0; i < CSK_IFACE_MAX; i++) {
        k_msgq_init(&s_ec718_handle.ack_msgq[i], &ack_msgq_buffer[i][0], sizeof(uint32_t), 10);
        // k_sem_init(&s_ec718_handle.ack_sem[i], 0, 1);
    }

    // k_mem_slab_init(&big_slab, big_slab_buffer, MAX_BIG_FRAME_SIZE, MAX_BIG_FRAME_NUMS);
    // k_mem_slab_init(&middle_slab, middle_slab_buffer, MAX_MIDDLE_FRAME_SIZE, MAX_MIDDLE_FRAME_NUMS);
    // k_mem_slab_init(&little_slab, little_slab_buffer, MAX_LITTLE_FRAME_SIZE, MAX_LITTLE_FRAME_NUMS);

    uart_irq_callback_user_data_set(s_ec718_handle.dev, uart_rx_callback, NULL);
	uart_irq_rx_enable(s_ec718_handle.dev);
    proto_socket_init();
    return 0;
}

K_MUTEX_DEFINE(uart_tx_mutex);
int ec718_uart_write(uint8_t *data, uint16_t size)
{
    // LOG_HEXDUMP_ERR(data, size, "send:");
    k_mutex_lock(&uart_tx_mutex, K_FOREVER);
    int ret = uart_fifo_fill(s_ec718_handle.dev, data, size);
    k_mutex_unlock(&uart_tx_mutex);
    return ret;
}

static void ec718_uart_send_ack(csk_interface_type_e iface, uint16_t seq_num)
{
    uint8_t send_buf[FRAME_HEADER_SIZE];
    struct csk_payload_header *payload_header = (struct csk_payload_header *)send_buf;
    payload_header->if_type = iface;
    payload_header->tag = htole16(CSK_HEADER_TAG);
    payload_header->flags = CSK_HEADER_FLAG_ACK;
    payload_header->payload_len = 0;
    payload_header->seq_num = htole16(seq_num);
    payload_header->reserved = 0;
    payload_header->hdr_checksum = 0;
    payload_header->tot_checksum = 0;

    payload_header->hdr_checksum = htole16(compute_checksum((uint8_t *)payload_header, FRAME_HEADER_SIZE));
    payload_header->tot_checksum = htole16(compute_checksum((uint8_t *)payload_header, FRAME_HEADER_SIZE));
    int ret = ec718_uart_write(send_buf, FRAME_HEADER_SIZE);
    if (ret != FRAME_HEADER_SIZE) {
        __ASSERT(0, "check failed");
    }
}

int ec718_uart_send_frame_with_ack(csk_interface_type_e iface, uint8_t *buf, uint16_t len)
{
    int ret = 0;
    uint8_t *send_buf = k_malloc(FRAME_HEADER_SIZE + len);
    if (!send_buf) {
        __ASSERT(0, "no mem check failed");
        return -ENOMEM;
    }
    struct csk_payload_header *frame = (struct csk_payload_header *)send_buf;
    frame->if_type = iface;
    frame->tag = htole16(CSK_HEADER_TAG);
    frame->flags = CSK_HEADER_FLAG_ACK_REQ;
    frame->payload_len = htole16(len);
    frame->seq_num = htole16(s_tx_seq_num);
    frame->reserved = 0;
    frame->hdr_checksum = 0;
    frame->tot_checksum = 0;

    memcpy(frame->payload, buf, len);

    frame->hdr_checksum = htole16(compute_checksum((uint8_t *)frame, FRAME_HEADER_SIZE));
    frame->tot_checksum = htole16(compute_checksum((uint8_t *)frame, FRAME_HEADER_SIZE + len));

    int retrancation_cnt = 0;
    uint32_t ack_seq_num = 0;
    do {
        ret = ec718_uart_write(send_buf, FRAME_HEADER_SIZE + len);
        if (ret != (FRAME_HEADER_SIZE + len)) {
            __ASSERT(0, "check failed");
        }

        ret = k_msgq_get(&s_ec718_handle.ack_msgq[iface], &ack_seq_num, K_MSEC(MAX_ACK_WAIT_TIME_MS));
        if ((ret == 0) && (ack_seq_num == s_tx_seq_num)) {
            break;
        }
        printk("--------------------------------------------get ack failed %d  ,remind: %d\n",
                s_tx_seq_num, k_pipe_read_avail(&s_rx_pipe));
        __ASSERT(0, "lose ack");
        // while (k_msgq_num_used_get(&ack_seq_cache) > 0) {
        //     uint32_t temp;
        //     k_msgq_get(&ack_seq_cache, &temp, K_FOREVER);
        //     printk("%d   ", temp);
        // }
        // printk("\n");
    } while (retrancation_cnt++ < MAX_RETRANCATION_TIMES);

    if (ret == 0) {
        s_tx_seq_num++;
    } else {
        printk("----------- wait ack failed: %d\n", s_tx_seq_num);
    }
    k_free(send_buf);
    return ret;
}

int ec718_uart_send_frame_with_ack_v2(csk_interface_type_e iface, uint8_t *buf, uint16_t len)
{
    int ret = 0;
    uint8_t send_buf[FRAME_HEADER_SIZE] = {0};
    struct csk_payload_header *frame = (struct csk_payload_header *)send_buf;
    frame->if_type = iface;
    frame->tag = htole16(CSK_HEADER_TAG);
    frame->flags = CSK_HEADER_FLAG_ACK_REQ;
    frame->payload_len = htole16(len);
    frame->seq_num = htole16(s_tx_seq_num);
    frame->reserved = 0;
    frame->hdr_checksum = 0;
    frame->tot_checksum = 0;

    frame->hdr_checksum = htole16(compute_checksum((uint8_t *)frame, FRAME_HEADER_SIZE));
    frame->tot_checksum = htole16(compute_checksum(buf, len) + compute_checksum((uint8_t *)frame, FRAME_HEADER_SIZE));

    int retrancation_cnt = 0;
    uint32_t ack_seq_num = 0;
    do {
        ret = ec718_uart_write((uint8_t *)frame, FRAME_HEADER_SIZE);
        if (ret != FRAME_HEADER_SIZE) {
            __ASSERT(0, "check failed");
        }
        ret = ec718_uart_write(buf, len);
        if (ret != len) {
            __ASSERT(0, "check failed");
        }

        ret = k_msgq_get(&s_ec718_handle.ack_msgq[iface], &ack_seq_num, K_MSEC(MAX_ACK_WAIT_TIME_MS));
        if ((ret == 0) && (ack_seq_num == s_tx_seq_num)) {
            break;
        }
        printk("--------------------------------------------get ack failed %d  ,remind: %d\n",
                s_tx_seq_num, k_pipe_read_avail(&s_rx_pipe));
        __ASSERT(0, "lose ack");
        // while (k_msgq_num_used_get(&ack_seq_cache) > 0) {
        //     uint32_t temp;
        //     k_msgq_get(&ack_seq_cache, &temp, K_FOREVER);
        //     printk("%d   ", temp);
        // }
        // printk("\n");
    } while (retrancation_cnt++ < MAX_RETRANCATION_TIMES);

    if (ret == 0) {
        s_tx_seq_num++;
    } else {
        printk("----------- wait ack failed: %d\n", s_tx_seq_num);
    }

    return len;
}

K_THREAD_DEFINE(uart_rx_process_thread_id, 4096,
                uart_rx_process, NULL, NULL, NULL,
                3, 0, 0);

K_THREAD_DEFINE(rx_thread_id, 4096,
                rx_process, NULL, NULL, NULL,
                3, 0, 0);

K_THREAD_DEFINE(rx_socket_thread_id, 4096,
                rx_socket_process, NULL, NULL, NULL,
                3, 0, 0);

