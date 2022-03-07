#ifndef __LISA_HAL_HTTP__
#define __LISA_HAL_HTTP__
/*
 * 文件      : hai_wifi.c
 *

 * 本程序只供学习使用，未经作者许可，不得用于其它任何用途
 * ListenAI BK7251开发板
 * 提供给应用层的wifi开发API
 * 包括 lisa_wifi_connect()  lisa_wifi_disconnect等
 * 官方网站:http://listenai.com/
 * 作者：   Alexho
 * 创建日期:2020.11.23
 * 版权所有，盗版必究。
 * Copyright(C) 安徽聆思智能科技有限公司
 * All rights reserved
 * ********************************************************************************
 *
 * 修改说明
 * V1.1 20201123
 * 1 . 创建。
 * 2 . 封装lisa的http/https接口
*/

#include "lisa_err.h"
#include "lisa_semaphore.h"
#include "lisa_thread.h"
#include <net/http_client.h>


#define URL_MAX_LENGTH 1024
#define BODY_MAX_LENGTH 1024

typedef enum {
	LISA_HTTP_OK = 0,
	LISA_HTTP_COMMON_ERR,
	LISA_HTTP_PARAM_ERROR,

} lisa_http_err_e;

typedef enum {
	LISA_HTTP_ON_ERROR,
	LISA_HTTP_ON_HEADER,  // 多次发生
	LISA_HTTP_ON_END,
} lisa_http_event_e;

typedef enum {
	LISA_HTTP_ERROR_PARAM,
	LISA_HTTP_ERROR_TIMEOUT,
	LISA_HTTP_ERROR_NETWORK,
} lisa_http_error_e;

typedef enum {
	LISA_HTTP_GET,
	LISA_HTTP_POST,
	LISA_HTTP_PUT,
	LISA_HTTP_PATCH,
	LISA_HTTP_DELETE,
} lisa_http_method_e;

typedef struct {
	const void *buf;
	int32_t len;
	void *user;
} lisa_http_data_t;

typedef struct {
	lisa_http_event_e what;
	lisa_http_error_e error;
	const char *header;
	uint16_t status_code;
} lisa_http_event_t;

typedef struct {
	lisa_http_method_e method;
	int sock_id;
	char url[URL_MAX_LENGTH];
	char host[URL_MAX_LENGTH];
	char body[BODY_MAX_LENGTH];
	const char **headers;
	int32_t body_len;
	uint32_t timeout_ms;
	void *user;
	void (*inter_on_event)(lisa_http_event_t *event);
	void (*inter_on_data)(lisa_http_data_t *data);
	uint16_t status_code;
} lisa_http_t;

typedef struct {
	lisa_http_method_e method;
	const char *url;
	const char *host;
	const char **headers;
	void *body;
	int32_t body_len;  // 不定长度传 -1
	uint32_t timeout_ms;
	void *user;
	void (*on_event)(lisa_http_event_t *event);
	void (*on_data)(lisa_http_data_t *data);
} lisa_http_request_t;
int lisa_http_create(void);
extern lisa_http_t *lisa_http_set(lisa_http_request_t *req);
lisa_http_err_e lisa_http_perform(lisa_http_t *ins);
// body=NULL, len=0 表示 EOF
lisa_http_err_e lisa_http_write(lisa_http_t *ins, const void *buffer, size_t length);

lisa_http_err_e lisa_http_cancel(lisa_http_t *ins);
lisa_http_err_e lisa_http_cleanup(void);

#endif  //__LISA_HAL_HTTP__