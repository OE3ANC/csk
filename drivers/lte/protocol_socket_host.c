#include "ec718_conn.h"

#include <zephyr/kernel.h>
#include <zephyr/net/net_ip.h>

#include "protocol_socket_host.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(protocol_socket, LOG_LEVEL_INF);

#define MAX_DEFAULT_TINEOUT_MS      1000
#define MAX_SOCKET_QUEUE_LEN        10

static uint32_t s_tx_max_timeout = MAX_DEFAULT_TINEOUT_MS;
static uint32_t s_rx_max_timeout = MAX_DEFAULT_TINEOUT_MS;

#define SOCKHEADER_ALLOC(command, args_size) ({ \
        int frame_size = sizeof(struct proto_sock_header) + args_size; \
        struct proto_sock_header *ptr = k_malloc(frame_size); \
            if(!ptr) { \
               LOG_ERR("no mem"); \
            } else { \
                ptr->ret = 0; \
                ptr->cmd = (command); \
            } \
        ptr; \
    })

#define SOCKHEADER_FREE(sock_header) do { \
        k_free(sock_header); \
    } while (0)

static struct k_msgq socket_cmd_msgq[PROTO_CMD_MAX];
static char socket_cmd_msgq_buffer[PROTO_CMD_MAX][sizeof(struct csk_payload_header *) * MAX_SOCKET_QUEUE_LEN];

static inline int proto_sock_send_frame(struct proto_sock_header *sock_header, uint16_t args_size)
{
    return ec718_uart_send_frame_with_ack_v2(SOCKET_IF, (uint8_t *)sock_header,
                                            PROTO_SOCK_HEADER_SIZE + args_size);
}

int proto_sock_rx_process(struct csk_payload_header *frame)
{
    struct proto_sock_header *sock_header = (struct proto_sock_header *)frame->payload;
    k_msgq_put(&socket_cmd_msgq[sock_header->cmd], frame, K_FOREVER);
    return 0;
}

int proto_sock_rx_process_v2(uint8_t *data, uint32_t size)
{
    struct csk_payload_header *frame = k_malloc(size);
    if (frame == NULL) {
        return -ENOMEM;
    }
    memcpy(frame, data, size);
    struct proto_sock_header *sock_header = (struct proto_sock_header *)frame->payload;
    k_msgq_put(&socket_cmd_msgq[sock_header->cmd], &frame, K_FOREVER);
    struct csk_payload_header *payload_header = (struct csk_payload_header *)frame;
    // printk("-------------- v2 send to process\n");
    return 0;
}

static int proto_sock_wait_rsp_v2(proto_socket_cmd_e cmd, void **out_frame, k_timeout_t timeout)
{
    struct csk_payload_header *frame = NULL;
    int ret = k_msgq_get(&socket_cmd_msgq[cmd], &frame, timeout);
    if ((ret != 0) || (frame == NULL)) {
        return ret;
    }
    struct proto_sock_header *sock_header = (struct proto_sock_header *)frame->payload;
    if (out_frame != NULL) {
        *out_frame = frame;
    }
    return sock_header->ret;
}

int proto_sock_socket(int family, int type, int proto)
{
    const int args_size = sizeof(struct socket_args);
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_SOCKET, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct socket_args *sock_args = (struct socket_args *)sock_header->args;
    sock_args->family = family;
    sock_args->type = type;
    sock_args->proto = proto;
    printk("---------soocket: %d %d %d\n", family, type, proto);
    proto_sock_send_frame(sock_header, args_size);
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, NULL, K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    printk("--- socket ret %d\n", ret);

    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_connect(int fd, const struct sockaddr *addr, uint32_t addr_len)
{
    const int args_size = sizeof(struct connect_args);
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_CONNECT, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct connect_args *connect_args = (struct connect_args *)sock_header->args;
    const struct sockaddr_in *z_sockaddr_in = (const struct sockaddr_in *)addr;
    connect_args->fd = fd;
    connect_args->addr_len = addr_len;
    connect_args->addr.sin_family = z_sockaddr_in->sin_family;
    connect_args->addr.sin_port = z_sockaddr_in->sin_port;
    connect_args->addr.sin_addr.s_addr = z_sockaddr_in->sin_addr.s_addr;  //Fixme: Only for ipv4

    proto_sock_send_frame(sock_header, args_size);
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, NULL,  K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_send(int fd, const void *data, uint32_t size, int flags)
{
    const int args_size = sizeof(struct send_args) + size;
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_SEND, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct send_args *send_args = (struct send_args *)sock_header->args;
    send_args->fd = fd;
    send_args->flags = flags;
    send_args->size = size;
    memcpy(send_args->data, data, size);

    proto_sock_send_frame(sock_header, args_size);
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, NULL,
                K_MSEC((s_tx_max_timeout > MAX_DEFAULT_TINEOUT_MS) ?
                        s_tx_max_timeout : MAX_DEFAULT_TINEOUT_MS));
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_sendto(int fd, const void *data, uint32_t size, int flags, const struct sockaddr *addr, uint32_t addr_len)
{
    const int args_size = sizeof(struct sendto_args) + size;
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_SENDTO, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct sendto_args *sendto_args = (struct sendto_args *)sock_header->args;
    const struct sockaddr_in *z_sockaddr_in = (const struct sockaddr_in *)addr;
    sendto_args->fd = fd;
    sendto_args->flags = flags;
    sendto_args->addr_len = addr_len;
    sendto_args->addr.sin_family = z_sockaddr_in->sin_family;
    sendto_args->addr.sin_port = z_sockaddr_in->sin_port;
    sendto_args->addr.sin_addr.s_addr = z_sockaddr_in->sin_addr.s_addr;  //Fixme: Only for ipv4
    sendto_args->size = size;
    memcpy(sendto_args->data, data, size);

    proto_sock_send_frame(sock_header, args_size);
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, NULL,
                K_MSEC((s_tx_max_timeout > MAX_DEFAULT_TINEOUT_MS) ?
                        s_tx_max_timeout : MAX_DEFAULT_TINEOUT_MS));
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_recv(int fd, void *data, uint32_t size, int flags)
{    
    const int args_size = sizeof(struct recv_args);
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_RECV, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct recv_args *recv_args = (struct recv_args *)sock_header->args;
    recv_args->fd = fd;
    recv_args->flags = flags;
    recv_args->size = size;

    proto_sock_send_frame(sock_header, args_size);
    struct csk_payload_header *frame = NULL;
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, (void **)&frame,
                K_MSEC((s_rx_max_timeout > MAX_DEFAULT_TINEOUT_MS) ?
                        s_tx_max_timeout : MAX_DEFAULT_TINEOUT_MS));
    if (ret > 0 && frame != NULL) {
        struct proto_sock_header *sock_header = (struct proto_sock_header *)frame->payload;
        struct recv_args *rsp_args = (struct recv_args *)sock_header->args;
        memcpy(data, rsp_args->data, ret);
    } else {
        printk("----recv failed  ret: %d\n", ret);
    }
    k_free(frame);
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_recvfrom(int fd, void *data, uint32_t size, int flags, const struct sockaddr *addr, uint32_t addr_len)
{
    const int args_size = sizeof(struct recvfrom_args);
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_RECVFROM, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct recvfrom_args *recvfrom_args = (struct recvfrom_args *)sock_header->args;
    const struct sockaddr_in *z_sockaddr_in = (const struct sockaddr_in *)addr;
    recvfrom_args->fd = fd;
    recvfrom_args->flags = flags;
    recvfrom_args->addr_len = addr_len;
    recvfrom_args->addr.sin_family = z_sockaddr_in->sin_family;
    recvfrom_args->addr.sin_port = z_sockaddr_in->sin_port;
    recvfrom_args->addr.sin_addr.s_addr = z_sockaddr_in->sin_addr.s_addr;  //Fixme: Only for ipv4
    recvfrom_args->size = size;

    proto_sock_send_frame(sock_header, args_size);
    struct csk_payload_header *frame = NULL;
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, (void **)&frame,
                K_MSEC((s_rx_max_timeout > MAX_DEFAULT_TINEOUT_MS) ?
                        s_tx_max_timeout : MAX_DEFAULT_TINEOUT_MS));
    if (ret > 0 && frame != NULL) {
        struct proto_sock_header *sock_header = (struct proto_sock_header *)frame->payload;
        struct recvfrom_args *rsp_args = (struct recvfrom_args *)sock_header->args;
        memcpy(data, rsp_args->data, ret);
    }
    k_free(frame);
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_set_socketopt(int fd, int level, proto_sock_sockopt_e optname, const void *optval, uint32_t optlen)
{
    const int args_size = sizeof(struct socketopt_args) + optlen;
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_SETSOCKETOPT, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    int ret = 0;
    struct socketopt_args *socketopt_args = (struct socketopt_args *)sock_header->args;
    socketopt_args->fd = (uint32_t)fd;
    socketopt_args->level = (uint32_t)level;
    socketopt_args->optname = (uint32_t)optname;
    socketopt_args->optlen = optlen;

    if (optname == PROTO_SOCK_SNDTIMEO || optname == PROTO_SOCK_RCVTIMEO) {
        struct proto_sock_timeval *in_timeval = (struct proto_sock_timeval *)optval;
        struct proto_sock_timeval *out_timeval = (struct proto_sock_timeval *)socketopt_args->optval;
        out_timeval->tv_sec = in_timeval->tv_sec;
        out_timeval->tv_usec = in_timeval->tv_usec;
    } else if (optname == PROTO_SOCK_KEEPALIVE) {
        struct proto_sock_commval *in_val = (struct proto_sock_commval *)optval;
        struct proto_sock_commval *out_val = (struct proto_sock_commval *)socketopt_args->optval;
        out_val->optval = in_val->optval;
    } else {
        ret = -ENOTSUP;
        goto __cleanup;
    }

    proto_sock_send_frame(sock_header, args_size);
    ret = proto_sock_wait_rsp_v2(sock_header->cmd, NULL, K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    if (ret == 0) {
         struct proto_sock_timeval *in_timeval = (struct proto_sock_timeval *)optval;
        if (optname == PROTO_SOCK_SNDTIMEO) {
            s_tx_max_timeout = in_timeval->tv_sec * 1000 + in_timeval->tv_usec / 1000 + MAX_DEFAULT_TINEOUT_MS;
        } else if (optname == PROTO_SOCK_RCVTIMEO) {
            s_rx_max_timeout = in_timeval->tv_sec * 1000 + in_timeval->tv_usec / 1000 + MAX_DEFAULT_TINEOUT_MS;
        }
    }

__cleanup:
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_close(int fd)
{
    const int args_size = sizeof(struct close_args);
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_CLOSE, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct close_args *close_args = (struct close_args *)sock_header->args;
    close_args->fd = fd;

    proto_sock_send_frame(sock_header, args_size);
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, NULL, K_MSEC(MAX_DEFAULT_TINEOUT_MS));

    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_getaddrinfo(const char *node, const char *service,
                            struct proto_sock_addrinfo *hint, struct proto_sock_addrinfo **res)
{
    int node_len = 0;
    int service_len = 0;
    if (node != NULL) {
        node_len = strlen(node);
    }
    if (service != NULL) {
        service_len = strlen(service);
    }
    const int args_size = sizeof(struct getaddrinfo_args) + node_len + service_len;
    struct proto_sock_header *sock_header = SOCKHEADER_ALLOC(PROTO_GETADDRINFO, args_size);
    if (sock_header == NULL) {
        return -ENOMEM;
    }

    struct getaddrinfo_args *getaddrinfo_args = (struct getaddrinfo_args *)sock_header->args;
    getaddrinfo_args->node_len = node_len;
    getaddrinfo_args->service_len = service_len;
    getaddrinfo_args->res_num = 0; //no use
    memcpy(&getaddrinfo_args->hint, hint, sizeof(struct proto_sock_addrinfo));
    int offset = 0;
    memcpy(&getaddrinfo_args->data[offset], node, node_len);
    offset += node_len;
    if (service_len > 0) {
        memcpy(&getaddrinfo_args->data[offset], service, service_len);
        offset += service_len;
    }

    proto_sock_send_frame(sock_header, args_size);
    struct csk_payload_header *rsp_frame = NULL;
    int ret = proto_sock_wait_rsp_v2(sock_header->cmd, (void **)&rsp_frame, K_MSEC(MAX_DEFAULT_TINEOUT_MS));
    if (ret == 0 && rsp_frame != NULL) {
        struct proto_sock_header *out_sock = (struct proto_sock_header *)rsp_frame->payload;
        struct getaddrinfo_args *out_args = (struct getaddrinfo_args *)out_sock->args;
        struct proto_sock_addrinfo *copy_res = k_calloc(out_args->res_num, sizeof(struct proto_sock_addrinfo));
        if (copy_res == NULL) {
            ret = -ENOMEM;
            goto __cleanup;
        }
        struct proto_sock_addrinfo *out_res_array = (struct proto_sock_addrinfo *)&out_args->data[offset];
        for (int i = 0; i < out_args->res_num; i++) {
            memcpy(&copy_res[i], &out_res_array[i], sizeof(struct proto_sock_addrinfo));
            copy_res[i].ai_next = &copy_res[i+1];
        }
        copy_res[out_args->res_num - 1].ai_next = NULL;
        *res = copy_res;
        k_free(rsp_frame);
    }

__cleanup:
    SOCKHEADER_FREE(sock_header);
    return ret;
}

int proto_sock_freeaddrinfo(struct proto_sock_addrinfo *res)
{
    k_free(res);
    return 0;
}

int proto_socket_init(void)
{
    for (int i = 0; i < PROTO_CMD_MAX; i++) {
        k_msgq_init(&socket_cmd_msgq[i], &socket_cmd_msgq_buffer[i][0],
                    sizeof(struct csk_payload_header *), MAX_SOCKET_QUEUE_LEN);
    }
    return 0;
}
