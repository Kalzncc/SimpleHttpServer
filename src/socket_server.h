#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "utils/http_msg.h"
#include "utils/url.h"

#define HTTP                   1
#define HTTPS                  2
#define port_t                 unsigned short
#define SHORT_BUF              128
#define CHUNKED_MAX_BUF        1024*128
#define SSL_CHUNKED_MAX_BUF    1024*128
#define SINGLE_MAX_BUF         8192

struct server_info {
/*
    Requried config to create a http server
    scheme : HTTP or HTTPS or HTTP | HTTPS 
    http(s)_listen_port : The port listened;
    handle_req : The call back function to handle client request.
                scheme : The scheme of current request : 0:http, 1:https;
                req    : The req message of current request;
                res    : The response message needed to send to client;
*/
    int scheme; 
    port_t http_listen_port;
    port_t https_listen_port;
    void* (*handle_req)(int scheme, struct http_req_msg *req, struct http_res_msg *res, void *super_info);
    void *super_info;
};

struct req_handle_info {
/*
    Requried config to handle a request
*/
    struct server_info *info;
    SSL_CTX *ctx;
    int connfd;
};

/*
    Start a http server following the server_info.
*/
void run_http_server(struct server_info * info);

#endif