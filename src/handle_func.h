#ifndef HANDLE_FUNC_H
#define HANDLE_FUNC_H

#include <stdlib.h>
#include "socket_server.h"
#include "utils/http_msg.h"
#include "utils/debug.h"

#define MAXLINE   4096

int say_hello(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int say_goodbye(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int say_input(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int say_json(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int download_file(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int download_big_file(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int method_check_do_get(const struct http_req_msg *request, struct http_res_msg *response);
int method_check_do_post(const struct http_req_msg *request, struct http_res_msg *response);
int method_check(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int redirect_https_req(const struct http_req_msg *request, struct http_res_msg *response);
int redirect(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
void get_range(const char *buf, size_t *start_p, size_t *end_p);
int video_mp4(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int video(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int for_test_py_index(int scheme, const struct http_req_msg *request, struct http_res_msg *response);
int for_test_py_dir(int scheme, const struct http_req_msg *request, struct http_res_msg *response);



#endif