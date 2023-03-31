#include "debug.h"




void _print_http_req_msg(const struct http_req_msg *msg) {
#ifdef __DEBUG
    printf("<< Http Request Message >> :[\n");
    printf("\t%s %s %s\n", msg->header.method, msg->header.url, msg->header.version);
    for (int i = 0; i < msg->header.field_cnt; i++) {
        printf("\t%s :%s\n", msg->header.fields[i].key, msg->header.fields[i].value);
    }
    printf("\t<body %d>]:End\n", (int)msg->body_len);
#endif
}
void _print_http_res_msg(const struct http_res_msg *msg) {
#ifdef __DEBUG
    printf("<< Http Response Message >> :[\n");
    printf("\t%s %s %s\n", msg->header.version, msg->header.code, msg->header.msg);
    for (int i = 0; i < msg->header.field_cnt; i++) {
        printf("\t%s :%s\n", msg->header.fields[i].key, msg->header.fields[i].value);
    }
    printf("\t<body %d>]:End\n", (int)msg->body_len);
#endif
}

void pass() {}