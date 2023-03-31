#ifndef DEBUG_H
#define DEBUG_H

#include "http_msg.h"

#ifdef __DEBUG
    #define _printf(...)           printf(__VA_ARGS__);
#else
    #define _printf(...)           pass()
#endif


void _print_http_req_msg(const struct http_req_msg *msg);
void _print_http_res_msg(const struct http_res_msg *msg);
void pass();



#endif