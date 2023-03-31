#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "socket_server.h"


#define MAX_URL_BIND     1024



struct http_url_bind {
/*
    Url bind configuration. bind a url to a handle function.
    You can return the status code.
*/
    char *url;
    int (*handle)(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
};

struct http_server_info {
/*
    Requried info to create a http server.(You needn't set the server_info.handle_info.)
    You need to bind a series of url to hanle function.
*/
    struct server_info server_info;
    int urls_cnt;
    struct http_url_bind* binds;
};

void init_http_server_info(struct http_server_info *info);
int bind_url(struct http_server_info *info, const char *url, int(*handle)(int scheme, const struct http_req_msg *request, struct http_res_msg *response));
int start_http_service(struct http_server_info *info);

#endif