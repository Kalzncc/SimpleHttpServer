#include "http_server.h"


int match_url(const char *req_url, const char *server_url) {
    int ser_len = strlen(server_url);
    int req_len = strlen(req_url);
    char buf[req_len + 1];
    strcpy(buf, req_url);
    if (server_url[ser_len-1] != '*') {
        return strcmp(buf, server_url) == 0;
    }
    if (ser_len > req_len) return 0;
    buf[ser_len - 1] = '*';
    buf[ser_len] = '\0';
    return strcmp(buf, server_url) == 0;
}

void* handle_req(int scheme, struct http_req_msg *req, struct http_res_msg *res, void *super_info) {
    struct http_server_info * info = (struct http_server_info *)super_info;
    set_res_version(res, req->header.version);
    set_res_field(res, "Content-Type", "text/plain;charset=UTF-8");
    set_res_field(res, "Keep-Alive", "timeout=6000");
    set_res_field(res, "Connection","keep-alive");
    for (int i = 0; i < info->urls_cnt; i++) {
        if (match_url(req->header.url, info->binds[i].url) == 0) continue;
        int status = info->binds[i].handle(scheme, req, res);
        
        if (status == 200) {
            set_res_code(res, "200");
            set_res_msg(res, "OK");
            char buf[32];
            sprintf(buf, "%u", (int)res->body_len);
            add_res_field(res, "Content-Length", buf);
        } else if (status == 301) {
            set_res_code(res, "301");
            set_res_msg(res, "Moved Permanently");
        } else if (status == 206) {
            set_res_code(res, "206");
            set_res_msg(res, "Partial Content");
        } else if (status == 404){
            set_res_code(res, "404");
            set_res_msg(res, "Not Found");
        } else {
            char buf[16] = { 0 };
            sprintf(buf, "%d", status);
            set_res_code(res, buf);
        }
        return NULL;
    }
    set_res_code(res, "404");
    set_res_msg(res, "Not Found");
    set_res_body(res, "404 Not Found");
    return NULL;
}

void init_http_server_info(struct http_server_info *info) {
    memset(info, 0, sizeof(struct http_server_info));
    info->binds = (struct http_url_bind *)malloc(MAX_URL_BIND * sizeof(struct http_url_bind));
    info->server_info.super_info = info;
    info->server_info.handle_req = handle_req;
}


int bind_url(struct http_server_info *info, const char *url, int(*handle)(int scheme, const struct http_req_msg *request, struct http_res_msg *response)) {
    if (info->urls_cnt == MAX_URL_BIND) 
        return -1;
    info->binds[info->urls_cnt].url = (char *)malloc(strlen(url) * sizeof(char) + 1);
    strcpy(info->binds[info->urls_cnt].url, url);
    info->binds[info->urls_cnt].handle = handle;
    info->urls_cnt++;
    return 0;
}


int start_http_service(struct http_server_info *info) {
    run_http_server(&(info->server_info));
    return 0;
}



