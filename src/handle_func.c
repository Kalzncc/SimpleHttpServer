#include "handle_func.h"

int say_hello(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_body(response, "Hello World!");
    return 200;
}
int say_goodbye(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_body(response, "Goodbye World!");
    return 200;
}
int say_input(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_body(response, request->header.url);
    return 200;
}

int say_json(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    
    set_res_field(response, "Content-Type", "text/json;charset=UTF-8");
    set_res_body(response, "{\"msg\":\"success\", \"code\":200, \"obj\":{\"res\":\"hello world!\"} }");
    return 200;
}
int download_file(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_field(response, "Content-Type", "imag/png;charset=UTF-8");
    set_res_field(response, "Content-disposition","attachment;filename=ying.png");
    FILE *fp = fopen("data/ying.png", "rb");
    if (fp ==  NULL){ 
        printf("Error");
        return 1;
    }
    char buf[1024];
    char body[MESSAGE_MAX_BUFF];
    int n, sd = 0;
    while((n = fread(buf, sizeof(char), 1024, fp)) > 0) {
        memcpy(body+sd, buf, n);
        sd += n;
    }
    sprintf(buf, "%d", sd);
    set_res_field(response, "Content-Length", buf);
    set_bin_res_body(response, body, sd);
    return 200;
}
int download_big_file(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_field(response, "Content-Type", "imag/png;charset=UTF-8");
    set_res_field(response, "Content-disposition","attachment;filename=ying.png");
    set_res_msg(response, "Socket-Server-Chunked:data/ying.png");
    return 700;
}


int method_check_do_get(const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_body(response, "Now Get");
    return 200;
}
int method_check_do_post(const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_body(response, "Now Post");
    return 200;
}
int method_check(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    if (strcmp("GET", request->header.method) == 0) {
        return method_check_do_get(request, response);
    } 
    if (strcmp("POST", request->header.method) == 0) {
        return method_check_do_post(request, response);
    }
    return 405;
}
int redirect_https_req(const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_code(response, "301");
    set_res_msg(response, "Moved Permanently");
    char location[128];
    strcpy(location, "https://10.0.0.1");
    strcat(location, request->header.url);
    set_res_field(response, "Location", location);
    return 301;
}
int redirect(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    printf("%d\n", scheme);
    if (scheme != HTTPS) {
        return redirect_https_req(request, response);
    } else {
        set_res_body(response, "Now HTTPS.");
        return 200;
    }
}
void get_range(const char *buf, size_t *start_p, size_t *end_p) {
    *start_p = atol(buf+6);
    int pos = 6;
    while(buf[++pos] != '-');
    if (buf[pos+1] == '\0') {
        *end_p = -1;
    } else {
        *end_p = atol(buf+pos+1);
    }
}


int video_mp4(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    if (strcmp(request->header.method, "HEAD") == 0)
        return 200;
    size_t start_p, end_p;
    char buf[128];
    if (find_req_field(request, "Range", buf) < 0) {
        start_p = 0;
    } else {
        get_range(buf, &start_p, &end_p);
    }
    sprintf(buf, "Socket-Server-Chunked:data/video.mp4 %ld", start_p);
    set_res_msg(response, buf);
    set_res_field(response, "Content-Type", "video/mp4");
    set_res_field(response, "Keep-Alive", "timeout=60");
    return 701;
}

int video(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_msg(response, "Socket-Server-Chunked:data/video.avi");
    set_res_field(response, "Content-Type", "video/mpeg");
    set_res_field(response, "Keep-Alive", "timeout=60");
    return 700;
}








int for_test_py_index(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    if (scheme == HTTP) {
        return redirect_https_req(request, response);
    }
    char range_buf[128];
    char body_buf[MESSAGE_MAX_BUFF];
    if (find_req_field(request, "Range", range_buf) == 0) {
        size_t start_p = 0, end_p = 0;
        get_range(range_buf, &start_p, &end_p);
        printf("<%ld %ld>\n", start_p, end_p);
        FILE *file = fopen("index.html", "r");
        if (file == NULL) {
            return 404;
        }
        fseek(file, start_p, SEEK_SET);
        size_t len;
        if (end_p != -1) {
            len = fread(body_buf, 1, end_p-start_p+1, file);
        } else {
            len = fread(body_buf, 1, MESSAGE_MAX_BUFF, file);
        }
        set_bin_res_body(response, body_buf, len);
        return 206;
    } else {
        FILE *file = fopen("index.html", "r");
        if (file == NULL) {
            return 404;
        }
        size_t len = fread(body_buf, 1, MESSAGE_MAX_BUFF, file);
        set_bin_res_body(response, body_buf, len);
        return 200;
    }
}

int for_test_py_dir(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    char path_buf[128];
    strcpy(path_buf, request->header.url);
    path_buf[4] = '\0';
    if (strcmp(path_buf, "/dir")) {
        return 404;
    }
    path_buf[4] = '/';
    FILE *file = fopen(path_buf+1, "r");
    if (file == NULL) {
        return 404;
    }
    char body_buf[MESSAGE_MAX_BUFF];
    size_t len = fread(body_buf, 1, MESSAGE_MAX_BUFF, file);
    set_bin_res_body(response, body_buf, len);
    return 200;
}