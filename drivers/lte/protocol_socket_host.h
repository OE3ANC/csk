#pragma once

#include "protocol_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

int proto_sock_rx_process(struct csk_payload_header *frame);
int proto_sock_rx_process_v2(uint8_t *data, uint32_t size);

int proto_socket_init(void);
int proto_sock_socket(int family, int type, int proto);
int proto_sock_connect(int fd, const struct sockaddr *addr, uint32_t addr_len);
int proto_sock_send(int fd, const void *data, uint32_t size, int flags);
int proto_sock_sendto(int fd, const void *data, uint32_t size, int flags, const struct sockaddr *addr, uint32_t addr_len);
int proto_sock_recv(int fd, void *data, uint32_t size, int flags);
int proto_sock_recvfrom(int fd, void *data, uint32_t size, int flags, const struct sockaddr *addr, uint32_t addr_len);
int proto_sock_set_socketopt(int fd, int level, proto_sock_sockopt_e optname, const void *optval, uint32_t optlen);
int proto_sock_close(int fd);
int proto_sock_getaddrinfo(const char *node, const char *service, struct proto_sock_addrinfo *hint, struct proto_sock_addrinfo **res);
int proto_sock_freeaddrinfo(struct proto_sock_addrinfo *res);

#ifdef __cplusplus
}
#endif
