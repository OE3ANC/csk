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

#define MAX_RETRANCATION_TIMES                  5
#define MAX_ACK_WAIT_TIME_MS                    50

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
    int rx_state;
    struct k_fifo big_frame_fifo;
    frame_fifo_item_t frame[MAX_RX_FRAME_NUMS];
    struct k_fifo little_frame_fifo;
    frame_fifo_item_t little_frame[MAX_RX_LITTLE_FRAME_NUMS];
    struct k_pipe *ringbuf;
    frame_fifo_item_t *current_frame;
    struct k_msgq ack_msgq[CSK_IFACE_MAX];
} ec718_handle_t;

K_MSGQ_DEFINE(frame_msgq, sizeof(frame_fifo_item_t *), 10, 4);
K_MSGQ_DEFINE(sock_frame_msgq, sizeof(frame_fifo_item_t *), 10, 4);
static uint16_t s_tx_seq_num = 0;
static uint16_t s_rx_seq_num = 0;
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

static ec718_handle_t s_comm_handle = {
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
    k_fifo_put(&s_comm_handle.big_frame_fifo, in_frame);
}

void free_little_frame(void *frame)
{
    lit_cnt--;
    frame_fifo_item_t *in_frame = frame;
    in_frame->size = 0;
    memset(in_frame->data, 0, FRAME_HEADER_SIZE);
    k_fifo_put(&s_comm_handle.little_frame_fifo, in_frame);
}

frame_fifo_item_t *alloca_frame_item(uint16_t payload_size)
{
    frame_fifo_item_t *frame_item = NULL;
    if (payload_size == 0) {
        lit_cnt++;
        frame_item = k_fifo_get(&s_comm_handle.little_frame_fifo, K_NO_WAIT);
        if (!frame_item) {
            big_cnt++;
            frame_item = k_fifo_get(&s_comm_handle.big_frame_fifo, K_NO_WAIT);
        }
    } else {
        big_cnt++;
        frame_item = k_fifo_get(&s_comm_handle.big_frame_fifo, K_NO_WAIT);
    }
    return frame_item;
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

static int no_payload_frame_handler(struct csk_payload_header *frame)
{
    frame_fifo_item_t *no_payload_frame_item = alloca_frame_item(0);
    if (!no_payload_frame_item) {
        return -ENOMEM;
    }
    memcpy(no_payload_frame_item->data, frame, FRAME_HEADER_SIZE);
    no_payload_frame_item->size = FRAME_HEADER_SIZE;
    int ret = k_msgq_put(&frame_msgq, &no_payload_frame_item, K_NO_WAIT);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

static int payload_checksum_failed_handle(frame_fifo_item_t *fifo_item)
{
    //pass old header
    int ret = 0;
    fifo_item->size -= FRAME_HEADER_SIZE;
    memmove(fifo_item->data, &fifo_item->data[FRAME_HEADER_SIZE], fifo_item->size);

    while (fifo_item->size >= FRAME_HEADER_SIZE) {
        if (!is_valid_tag(fifo_item->data)) {
            fifo_item->size -= 1;
            memmove(fifo_item->data, &fifo_item->data[1], fifo_item->size);
            continue;
        }
        if (!is_valid_hdr_checksum(fifo_item->data)) {
            fifo_item->size -= 1;
            memmove(fifo_item->data, &fifo_item->data[1], fifo_item->size);
            continue;
        }
        struct csk_payload_header *payload_header = (struct csk_payload_header *)fifo_item->data;
        if (payload_header->payload_len != 0) {
            if (fifo_item->size >= payload_header->payload_len) {
                if (!is_valid_tot_checksum(fifo_item->data)) {
                    //payload still currupted, find next valid frame
                    fifo_item->size -= FRAME_HEADER_SIZE;
                    memmove(fifo_item->data, &fifo_item->data[FRAME_HEADER_SIZE], fifo_item->size);
                    continue;
                }
                frame_fifo_item_t *new_fifo_item = alloca_frame_item(payload_header->payload_len);
                if (new_fifo_item == NULL) {
                    fifo_item->size -= (FRAME_HEADER_SIZE + payload_header->payload_len);
                    memmove(fifo_item->data, &fifo_item->data[FRAME_HEADER_SIZE + payload_header->payload_len], fifo_item->size);
                    continue;
                }
                new_fifo_item->size = FRAME_HEADER_SIZE + payload_header->payload_len;
                memcpy(new_fifo_item->data, payload_header, new_fifo_item->size);
                ret = k_msgq_put(&frame_msgq, &new_fifo_item, K_NO_WAIT);
                // BaseType_t os_ret = xQueueSend(frame_queue, &new_fifo_item, 0);
                if (ret != 0) {
                    fifo_item->size -= (FRAME_HEADER_SIZE + payload_header->payload_len);
                    memmove(fifo_item->data, &fifo_item->data[FRAME_HEADER_SIZE + payload_header->payload_len], fifo_item->size);
                    continue;
                }
                fifo_item->size -= new_fifo_item->size;
                memmove(fifo_item->data, &fifo_item->data[new_fifo_item->size], fifo_item->size);
                continue;
            } else {
                ret = 1; //valid header, wait for payload
                break;
            }
        } else {
            no_payload_frame_handler(payload_header);
            fifo_item->size -= FRAME_HEADER_SIZE;
            memmove(fifo_item->data, &fifo_item->data[FRAME_HEADER_SIZE], fifo_item->size);
            continue;
        }
    }
    return ret;
}

static void uart_rx_process(void *arg1, void *arg2, void *arg3)
{
    int ret = 0;
    int bytes_read = 0;
    while (1) {
        k_sem_take(&s_uart_rx_sem, K_FOREVER);
        while (k_pipe_read_avail(&s_rx_pipe) > 0) {
            frame_fifo_item_t *fifo_item = s_comm_handle.current_frame;
            if (!fifo_item) {
                fifo_item = alloca_frame_item(MAX_FRAME_PAYLOAD_SIZE);
                if (!fifo_item) {
                    printk("no mem, data lose\n");
                    break;
                }
                s_comm_handle.current_frame = fifo_item;
            }
            if (s_comm_handle.rx_state == 0) {  //rx header
                ret = k_pipe_get(&s_rx_pipe, &fifo_item->data[fifo_item->size], FRAME_HEADER_SIZE - fifo_item->size, &bytes_read, 1, K_NO_WAIT);
                fifo_item->size += bytes_read;
                if (fifo_item->size >= FRAME_HEADER_SIZE) {
                    if (!is_valid_tag(fifo_item->data)) {
                        //no valid tag
                        fifo_item->size -= 1;
                        memmove(fifo_item->data, &fifo_item->data[1], fifo_item->size);
                        continue;
                    }
                    if (!is_valid_hdr_checksum(fifo_item->data)) {
                        //checksum failed
                        fifo_item->size -= 1;
                        memmove(fifo_item->data, &fifo_item->data[1], fifo_item->size);
                        continue;
                    }
                    struct csk_payload_header *payload_header = (struct csk_payload_header *)fifo_item->data;
                    if (payload_header->payload_len == 0) {  //frame without payload
                        no_payload_frame_handler(payload_header);  //Fixme: check ret value
                        fifo_item->size = 0;
                        s_comm_handle.rx_state = 0;
                    } else {
                        s_comm_handle.rx_state = 1;
                    }
                    continue;
                } else {
                    continue;  //no valid frame lens
                }
            }
            if (s_comm_handle.rx_state == 1) {  //rx payload
                struct csk_payload_header *payload_header = (struct csk_payload_header *)fifo_item->data;
                int to_read = payload_header->payload_len - (fifo_item->size - FRAME_HEADER_SIZE);
                ret = k_pipe_get(&s_rx_pipe, &fifo_item->data[fifo_item->size], to_read, &bytes_read, 1, K_NO_WAIT);
                fifo_item->size += bytes_read;
                if (fifo_item->size >= (payload_header->payload_len + FRAME_HEADER_SIZE)) {
                    if (!is_valid_tot_checksum(fifo_item->data)) {
                        s_comm_handle.rx_state = payload_checksum_failed_handle(fifo_item);
                        continue;
                    }
                    ret = k_msgq_put(&frame_msgq, &fifo_item, K_NO_WAIT);
                    if (ret != 0) {
                        printk("--frame fifo is full\n");
                    }
                    s_comm_handle.rx_state = 0;
                    s_comm_handle.current_frame = NULL;
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
            k_msgq_put(&s_comm_handle.ack_msgq[payload_header->if_type], &ack_seq_num, K_FOREVER);
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
    if (!device_is_ready(s_comm_handle.dev)) {
        printk("---------------- not ready\n");
        return -1;
    }

    s_comm_handle.rx_state = 0;
    k_fifo_init(&s_comm_handle.big_frame_fifo);
    k_fifo_init(&s_comm_handle.little_frame_fifo);
    for (int i = 0; i < MAX_RX_FRAME_NUMS; i++) {
        s_comm_handle.frame[i].data = k_malloc(MAX_FRAME_SIZE);
        if (!s_comm_handle.frame[i].data) {
            printk("----------- no mem\n");
            break;
        }
        s_comm_handle.frame[i].size = 0;
        s_comm_handle.frame[i].free_frame = free_big_frame;
        k_fifo_put(&s_comm_handle.big_frame_fifo, &s_comm_handle.frame[i]);
    }

    for (int i = 0; i < MAX_RX_LITTLE_FRAME_NUMS; i++) {
        s_comm_handle.little_frame[i].data = k_malloc(FRAME_HEADER_SIZE);
        if (!s_comm_handle.little_frame[i].data) {
            printk("----------- no mem\n");
            break;
        }
        s_comm_handle.little_frame[i].size = 0;
        s_comm_handle.little_frame[i].free_frame = free_little_frame;
        k_fifo_put(&s_comm_handle.little_frame_fifo, &s_comm_handle.little_frame[i]);
    }

    for (int i = 0; i < CSK_IFACE_MAX; i++) {
        k_msgq_init(&s_comm_handle.ack_msgq[i], &ack_msgq_buffer[i][0], sizeof(uint32_t), 10);
        // k_sem_init(&s_comm_handle.ack_sem[i], 0, 1);
    }

    // k_mem_slab_init(&big_slab, big_slab_buffer, MAX_BIG_FRAME_SIZE, MAX_BIG_FRAME_NUMS);
    // k_mem_slab_init(&middle_slab, middle_slab_buffer, MAX_MIDDLE_FRAME_SIZE, MAX_MIDDLE_FRAME_NUMS);
    // k_mem_slab_init(&little_slab, little_slab_buffer, MAX_LITTLE_FRAME_SIZE, MAX_LITTLE_FRAME_NUMS);

    uart_irq_callback_user_data_set(s_comm_handle.dev, uart_rx_callback, NULL);
	uart_irq_rx_enable(s_comm_handle.dev);
    proto_socket_init();
    return 0;
}

K_MUTEX_DEFINE(uart_tx_mutex);
int ec718_uart_write(uint8_t *data, uint16_t size)
{
    // LOG_HEXDUMP_ERR(data, size, "send:");
    k_mutex_lock(&uart_tx_mutex, K_FOREVER);
    int ret = uart_fifo_fill(s_comm_handle.dev, data, size);
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
    uint16_t max_delay_time = MAX_ACK_WAIT_TIME_MS;
    do {
        ret = ec718_uart_write(send_buf, FRAME_HEADER_SIZE + len);
        if (ret != (FRAME_HEADER_SIZE + len)) {
            __ASSERT(0, "check failed");
        }

        ret = k_msgq_get(&s_comm_handle.ack_msgq[iface], &ack_seq_num, K_MSEC(max_delay_time));
        if ((ret == 0) && (ack_seq_num == s_tx_seq_num)) {
            break;
        }
        printk("--------------------------------------------get ack failed %d  ,remind: %d\n",
                s_tx_seq_num, k_pipe_read_avail(&s_rx_pipe));
        // __ASSERT(0, "lose ack");
        max_delay_time *= 2;
        // while (k_msgq_num_used_get(&ack_seq_cache) > 0) {
        //     uint32_t temp;
        //     k_msgq_get(&ack_seq_cache, &temp, K_FOREVER);
        //     printk("%d   ", temp);
        // }
        // printk("\n");
    } while (++retrancation_cnt < MAX_RETRANCATION_TIMES);

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
    uint16_t max_delay_time = MAX_ACK_WAIT_TIME_MS;
    do {
        ret = ec718_uart_write((uint8_t *)frame, FRAME_HEADER_SIZE);
        if (ret != FRAME_HEADER_SIZE) {
            __ASSERT(0, "check failed");
        }
        ret = ec718_uart_write(buf, len);
        if (ret != len) {
            __ASSERT(0, "check failed");
        }

        ret = k_msgq_get(&s_comm_handle.ack_msgq[iface], &ack_seq_num, K_MSEC(max_delay_time));
        if ((ret == 0) && (ack_seq_num == s_tx_seq_num)) {
            break;
        }
        printk("--------------------------------------------get ack failed %d  ,remind: %d\n",
                s_tx_seq_num, k_pipe_read_avail(&s_rx_pipe));
        max_delay_time *= 2;
    } while (++retrancation_cnt < MAX_RETRANCATION_TIMES);

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

