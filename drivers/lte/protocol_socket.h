#pragma once

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(n)  (1UL << (n))
#endif

#define PROTO_SOCK_HEADER_SIZE  sizeof(struct proto_sock_header)

struct proto_sock_header { //8
    uint32_t cmd;
    int ret;
    uint8_t args[0];
} __attribute__((packed));

typedef enum {
    PROTO_SOCKET,
    PROTO_CONNECT,
    PROTO_SEND,
    PROTO_SENDTO,
    PROTO_RECV,
    PROTO_RECVFROM,
    PROTO_SETSOCKETOPT,
    PROTO_CLOSE,
    PROTO_GETADDRINFO,
	PROTO_CMD_MAX,
} proto_socket_cmd_e;

typedef enum {
    PROTO_SOCK_AF_UNSPEC,
    PROTO_SOCK_AF_INET,
    PROTO_SOCK_AF_INET6,
} proto_sock_family_e;

typedef enum {
    PROTO_SOCK_AI_STREAM = 1,
    PROTO_SOCK_AI_DGRAM,
} proto_sock_socktype_e;


typedef enum {
    PROTO_SOCK_IPPROTO_IP = 0,
    PROTO_SOCK_IPPROTO_TCP = 6,
    PROTO_SOCK_IPPROTO_UDP = 17,
} proto_sock_ipprotocol_e;

typedef enum {
    PROTO_SOCK_SNDTIMEO,
    PROTO_SOCK_RCVTIMEO,
    PROTO_SOCK_KEEPALIVE,
} proto_sock_sockopt_e;

struct socket_args {  //12
    int family;
    int type;
    int proto;
} __attribute__((packed));

struct proto_in_addr {
  uint32_t s_addr;
};

struct proto_sockaddr_in {
  uint8_t         sin_len;
  uint8_t         sin_family;
  uint16_t        sin_port;
  struct proto_in_addr  sin_addr;
  uint8_t         sin_zero[8];
} __attribute__((packed));

struct proto_sockaddr {
    uint16_t sa_family;
    uint8_t  sa_data[14];
} __attribute__((packed));

struct connect_args {
    int fd;
    uint32_t addr_len;
    struct proto_sockaddr_in addr;
} __attribute__((packed));

struct send_args {
    int fd;
    int flags;
    uint32_t size;
    uint8_t data[0];
} __attribute__((packed));

struct sendto_args {
    int fd;
    int flags;
    uint32_t addr_len;
    struct proto_sockaddr_in addr;
    uint32_t size;
    uint8_t data[0];
} __attribute__((packed));

struct recv_args {
    int fd;
    int flags;
    uint32_t size;
    uint8_t data[0];
} __attribute__((packed));

struct recvfrom_args {
    int fd;
    int flags;
    uint32_t addr_len;
    struct proto_sockaddr_in addr;
    uint32_t size;
    uint8_t data[0];
} __attribute__((packed));

struct proto_sock_timeval {
	uint64_t tv_sec;
	uint32_t tv_usec;
} __attribute__((packed));

struct proto_sock_commval {
	uint32_t optval;
} __attribute__((packed));

struct socketopt_args {
    int fd;
    int level;
    int optname;
    uint32_t optlen;
    uint8_t optval[0];
} __attribute__((packed));

struct close_args {
    int fd;
} __attribute__((packed));

typedef enum {
    PROTO_SOCK_AI_PASSIVE =     BIT(0),
    PROTO_SOCK_AI_NUMERICHOST = BIT(1),
} proto_sock_ai_flag_e;

struct proto_sock_addrinfo {
    int                          ai_flags;
    int                          ai_family;
    int                          ai_socktype;
    int                          ai_protocol;
    size_t                       ai_addrlen;
    struct proto_sockaddr        ai_addr;
    struct proto_sock_addrinfo  *ai_next;
} __attribute__((packed));

struct getaddrinfo_args {
    uint16_t node_len;
    uint16_t service_len;
    struct proto_sock_addrinfo hint;
    uint16_t res_num;
    uint8_t data[0]; //  "node" + "service" + result
} __attribute__((packed));



#ifdef __cplusplus
}
#endif
