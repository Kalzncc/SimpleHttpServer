#ifndef HTTP_MSG_H
#define HTTP_MSG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "url.h"
#define FIELD_MAX_BUFF      1024
#define FIELD_MAX_COUNT     1024
#define MESSAGE_MAX_BUFF    1024*1024*2
struct http_header_field {
/*
    HTTP HEADER FIELD INFO
*/
    char * key;
    char * value;
};


struct http_req_header {
/*
    HTTP REQUEST HEADER
*/   
    char *method; /*GET POST DELETE PUT OPTION*/
    char *url;
    char *version;
    size_t field_cnt;
    struct http_header_field *fields;

};

struct http_req_msg {
/*
    HTTP REQUEST MESSAGE
*/   
    struct http_req_header header;
    char *body;    
    size_t body_len;
};

struct http_res_header {
/*
    HTTP RESPONSE HEADER
*/
    char *version;
    char *code;
    char *msg;
    size_t field_cnt;
    struct http_header_field *fields;
};

struct http_res_msg {
/*
    HTTP RESPONSE HEADER
*/
    struct http_res_header header;
    char *body;
    size_t body_len;
};

/*
    Parse a string to a http_req/s_msg object;
    If return val is 0, that's ok;
    If return val is -1, there are error(s) in proccess.
*/
int parse_http_req_msg(const char *http_req_str, size_t str_len, struct http_req_msg *msg);
int parse_http_res_msg(const char *http_req_str, size_t str_len, struct http_res_msg *msg);



/*
    Stringify a http_req/s_msg object to string 
    If return val is -1, there are error(s) in proccess;
    IF return val is non negative, that's ok, and the value is lengyh of string;
*/
size_t stringify_http_req_msg(char *http_req_str, const struct http_req_msg *msg);
size_t stringify_http_res_msg(char *http_res_str, const struct http_res_msg *msg);




void set_res_version(struct http_res_msg *msg, const char * version);
void set_res_code(struct http_res_msg *msg, const char * code);
void set_res_msg(struct http_res_msg *msg, const char * v_msg);
void set_res_field(struct http_res_msg *msg, const char *key, const char *value);
void add_res_field(struct http_res_msg *msg, const char *key, const char *value);
void set_res_body(struct http_res_msg *msg, const char *body);
void set_bin_res_body (struct http_res_msg *msg, const void *body, size_t size);

int find_req_field(const struct http_req_msg *msg, const char *key, char *value);
void find_chunked_range_info(const struct http_res_msg *msg, char *buf, size_t *file_seek);

void free_res_msg(struct http_res_msg *msg);
void free_req_msg(struct http_req_msg *msg);



#endif