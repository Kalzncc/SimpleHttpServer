#include "http_msg.h"



size_t get_field(char *buf, const char *http_msg_str, size_t str_size, size_t *fd, char sparator) {
    if (*fd >= str_size) return 0;
    
    if (http_msg_str[*fd] == (char)32 || http_msg_str[*fd] == (char)10) {
        buf[0] = http_msg_str[*fd];
        buf[1] = '\0';
        ++(*fd);
        return 1;
    }
    int len = 0;
    while(*fd < str_size) {
        if ( http_msg_str[*fd] == sparator) {
            buf[len] = '\0';
            ++(*fd);
            return len;
        }
        
        buf[len++] = http_msg_str[(*fd)++];
    }
    buf[len] = '\0';
    ++(*fd);
    return len * sizeof(char);
}
size_t get_field_end(char *buf, const char *http_msg_str, size_t str_size, size_t *fd) {
    if (*fd >= str_size) return 0;
    int len = 0;
    while(*fd < str_size) {        
        buf[len++] = http_msg_str[(*fd)++];
    }
    buf[len] = '\0';
    ++(*fd);
    return len * sizeof(char);
}


int parse_http_req_msg(const char *http_req_str, size_t str_len, struct http_req_msg *msg) {
    memset(msg, 0, sizeof(struct http_req_msg));
    char *buf = (char *)malloc(MESSAGE_MAX_BUFF * sizeof(char));
    size_t fd = 0;
    if (get_field(buf, http_req_str, str_len, &fd, (char)32) < 0)
        return -1;
    msg->header.method = (char *)malloc(strlen(buf) * sizeof(char) + 1);
    strcpy(msg->header.method, buf);
    
    if (get_field(buf, http_req_str, str_len, &fd, (char)32) < 0)
        return -1;
    urldecode(buf);
    msg->header.url = (char *)malloc(strlen(buf) * sizeof(char) + 1);
    strcpy(msg->header.url, buf);

    if (get_field(buf, http_req_str, str_len, &fd, (char)10) < 0)
        return -1;
    msg->header.version = (char *)malloc(strlen(buf) * sizeof(char) + 1);
    strcpy(msg->header.version, buf);

    struct http_header_field fields[FIELD_MAX_COUNT];
    int field_count = 0;
    while(1) {
        int sta = get_field(buf, http_req_str, str_len, &fd, (char)32);
        if (buf[0] == (char)10) break;
        if (sta < 0) return -1;
        buf[sta-1] = '\0';
        fields[field_count].key = (char *)malloc(strlen(buf) * sizeof(char) + 1);
        strcpy(fields[field_count].key, buf);        
        sta = get_field(buf, http_req_str, str_len, &fd, (char)10);
        if (sta < 0) return -1;
        
        fields[field_count].value = (char *)malloc(strlen(buf) * sizeof(char) + 1);
        strcpy(fields[field_count].value, buf);
        field_count++;
    }
    
    msg->header.field_cnt = field_count;
    msg->header.fields = (struct http_header_field *)malloc(field_count * sizeof(struct http_header_field));
    memcpy(msg->header.fields, fields, field_count * sizeof(struct http_header_field));

    size_t body_len = get_field_end(buf, http_req_str, str_len, &fd);
    if (body_len) {
        msg->body = (char *)malloc(body_len);
        memcpy(msg->body, buf, body_len);
        msg->body_len = body_len;
    } else {
        msg->body_len = 0;
        msg->body = NULL;
    }
    free(buf);
    return 0;
}
int parse_http_res_msg(const char *http_res_str, size_t str_len, struct http_res_msg *msg) {
    memset(msg, 0, sizeof(struct http_res_msg));
    char *buf = (char *)malloc(MESSAGE_MAX_BUFF * sizeof(char));
    size_t fd = 0;
    if (get_field(buf, http_res_str, str_len, &fd, ' ') < 0)
        return -1;
    msg->header.version = (char *)malloc(strlen(buf) * sizeof(char) + 1);
    
    strcpy(msg->header.version, buf);
    
    if (get_field(buf, http_res_str, str_len, &fd, ' ') < 0)
        return -1;
    msg->header.code = (char *)malloc(strlen(buf) * sizeof(char) + 1);
    strcpy(msg->header.code, buf);

    if (get_field(buf, http_res_str, str_len, &fd, '\n') < 0)
        return -1;
    msg->header.msg = (char *)malloc(strlen(buf) * sizeof(char) + 1);
    strcpy(msg->header.msg, buf);

    struct http_header_field fields[FIELD_MAX_COUNT];
    int field_count = 0;
    while(1) {
        int sta = get_field(buf, http_res_str, str_len, &fd, ' ');
        if (buf[0] == (char)10) break;
        if (sta < 0) return -1;
        buf[sta-1] = '\0';
        fields[field_count].key = (char *)malloc(strlen(buf) * sizeof(char) + 1);
        strcpy(fields[field_count].key, buf);        

        sta = get_field(buf, http_res_str, str_len, &fd, '\n');
        if (sta < 0) return -1;
        fields[field_count].value = (char *)malloc(strlen(buf) * sizeof(char) + 1);
        strcpy(fields[field_count].value, buf);
        field_count++;
    }
    msg->header.field_cnt = field_count;
    msg->header.fields = (struct http_header_field *)malloc(field_count * sizeof(struct http_header_field));
    memcpy(msg->header.fields, fields, field_count * sizeof(struct http_header_field));

    size_t body_len = get_field_end(buf, http_res_str, str_len, &fd);
    if (body_len) {
        msg->body = (char *)malloc(body_len);
        memcpy(msg->body, buf, body_len);
        msg->body_len = body_len;
    } else {
        msg->body_len = 0;
        msg->body = NULL;
    }
    free(buf);
    return 0;
}


size_t stringify_http_req_msg(char *http_req_str, const struct http_req_msg *msg) {
    http_req_str[0] = '\0';
    strcpy(http_req_str, msg->header.method);
    strcat(http_req_str, " ");



    char buf[FIELD_MAX_BUFF];
    strcpy(buf, msg->header.url);
    urlencode(buf);
    strcat(http_req_str, buf);
    
    
    
    strcat(http_req_str, " ");
    strcat(http_req_str, msg->header.version);
    strcat(http_req_str, "\r\n");
    for (size_t i = 0; i < msg->header.field_cnt; i++) {
        strcat(http_req_str, msg->header.fields[i].key);
        strcat(http_req_str, ": ");
        strcat(http_req_str, msg->header.fields[i].value);
        strcat(http_req_str, "\r\n");
    }
    strcat(http_req_str, "\r\n");
    int str_len = strlen(http_req_str);
    if (msg->body !=NULL) {
        memcpy(http_req_str + str_len * sizeof(char), msg->body,  msg->body_len);
    }
    return str_len * sizeof(char) + msg->body_len;
}
size_t stringify_http_res_msg(char *http_res_str, const struct http_res_msg *msg) {
    http_res_str[0] = '\0';
    strcpy(http_res_str, msg->header.version);
    strcat(http_res_str, " ");
    strcat(http_res_str, msg->header.code);
    strcat(http_res_str, " ");
    strcat(http_res_str, msg->header.msg);
    strcat(http_res_str, "\r\n");

    for (size_t i = 0; i < msg->header.field_cnt; i++) {
        strcat(http_res_str, msg->header.fields[i].key);
        strcat(http_res_str, ": ");
        strcat(http_res_str, msg->header.fields[i].value);
        strcat(http_res_str, "\r\n");
    }
    strcat(http_res_str, "\r\n");


    int str_len = strlen(http_res_str);
    if (msg->body !=NULL) {
        memcpy(http_res_str + str_len * sizeof(char), msg->body,  msg->body_len);
    }
    return str_len * sizeof(char) + msg->body_len;
}



void set_res_version(struct http_res_msg *msg, const char * version) {
    if (msg->header.version != NULL)
        free(msg->header.version);
    msg->header.version = (char *)malloc(strlen(version) * sizeof(char) + 1);
    strcpy(msg->header.version, version);
}
void set_res_code(struct http_res_msg *msg, const char * code) {
    if (msg->header.code != NULL)
        free(msg->header.code);
    msg->header.code = (char *)malloc(strlen(code) * sizeof(char) + 1);
    strcpy(msg->header.code, code);
}
void set_res_msg(struct http_res_msg *msg, const char * v_msg) {
    if (msg->header.msg != NULL)
        free(msg->header.msg);
    msg->header.msg = (char *)malloc(strlen(v_msg) * sizeof(char) + 1);
    strcpy(msg->header.msg, v_msg);
}
void set_res_field(struct http_res_msg *msg, const char *key, const char *value) {
    
    for (int i = 0; i < msg->header.field_cnt; i++) {
        if (strcmp(key, msg->header.fields[i].key) != 0) continue;
        free(msg->header.fields[i].value);
        msg->header.fields[i].value = (char *)malloc(strlen(value) * sizeof(char) + 1);
        strcpy(msg->header.fields[i].value, value);
        return;
    }
    if (msg->header.field_cnt) {
        struct http_header_field *field_buf = (struct http_header_field *)malloc(msg->header.field_cnt * sizeof(struct http_header_field));
        memcpy(field_buf, msg->header.fields, msg->header.field_cnt * sizeof(struct http_header_field));
        free(msg->header.fields);
        msg->header.fields = (struct http_header_field *)malloc((msg->header.field_cnt+1) * sizeof(struct http_header_field));
        memcpy(msg->header.fields, field_buf, msg->header.field_cnt * sizeof(struct http_header_field));
        free(field_buf);
    } else {
        msg->header.fields = (struct http_header_field *)malloc(sizeof(struct http_header_field));
    }
    msg->header.fields[msg->header.field_cnt].key = (char *)malloc(strlen(key) * sizeof(char) + 1);
    strcpy(msg->header.fields[msg->header.field_cnt].key, key);
    msg->header.fields[msg->header.field_cnt].value = (char *)malloc(strlen(value) * sizeof(char) + 1);
    strcpy(msg->header.fields[msg->header.field_cnt].value, value);
    msg->header.field_cnt++;
}

void add_res_field(struct http_res_msg *msg, const char *key, const char *value) {
    for (int i = 0; i < msg->header.field_cnt; i++) {
        if (strcmp(key, msg->header.fields[i].key) != 0) continue;
        return;
    }
    if (msg->header.field_cnt) {
        struct http_header_field *field_buf = (struct http_header_field *)malloc(msg->header.field_cnt * sizeof(struct http_header_field));
        memcpy(field_buf, msg->header.fields, msg->header.field_cnt * sizeof(struct http_header_field));
        free(msg->header.fields);
        msg->header.fields = (struct http_header_field *)malloc((msg->header.field_cnt+1) * sizeof(struct http_header_field));
        memcpy(msg->header.fields, field_buf, msg->header.field_cnt * sizeof(struct http_header_field));
        free(field_buf);
    } else {
        msg->header.fields = (struct http_header_field *)malloc(sizeof(struct http_header_field));
    }
    
    msg->header.fields[msg->header.field_cnt].key = (char *)malloc(strlen(key) * sizeof(char) + 1);
    strcpy(msg->header.fields[msg->header.field_cnt].key, key);
    msg->header.fields[msg->header.field_cnt].value = (char *)malloc(strlen(value) * sizeof(char) + 1);
    strcpy(msg->header.fields[msg->header.field_cnt].value, value);
    msg->header.field_cnt++;
}
void set_bin_res_body (struct http_res_msg *msg, const void *body, size_t size) {
    free(msg->body);
    msg->body = (char *)malloc(size);
    msg->body_len = size;
    memcpy(msg->body, body, size);
    
}
void set_res_body(struct http_res_msg *msg, const char *body) {
    size_t size = strlen(body);
    set_bin_res_body(msg, body, size);
}

int find_req_field(const struct http_req_msg *msg, const char *key, char *value) {
    for (int i = 0; i < msg->header.field_cnt; i++) {
        if (strcmp(key, msg->header.fields[i].key) != 0) continue;
        strcpy(value, msg->header.fields[i].value);
        return 0;
    }
    return -1;
}
void find_chunked_range_info(const struct http_res_msg *msg, char *buf, size_t *file_seek) {
    char *path = msg->header.msg + 22;
    char *seek_pos = path;
    for (; *seek_pos != ' '; seek_pos++);
    *seek_pos = '\0';
    ++seek_pos;
    *file_seek = atol(seek_pos);
    strcpy(buf, path);
}

void free_res_msg(struct http_res_msg *msg) {
    if (msg == NULL)
        return;
    if (msg->body != NULL)
        free(msg->body);
    if (msg->header.code != NULL)
        free(msg->header.code);
    if (msg->header.version != NULL)
        free(msg->header.version);
    if (msg->header.msg != NULL)
        free(msg->header.msg);
    for (int i = 0; i < msg->header.field_cnt; i++) {
        free(msg->header.fields[i].key);
        free(msg->header.fields[i].value);
    }
    free(msg->header.fields);
}
void free_req_msg(struct http_req_msg *msg) {
    if (msg == NULL)
        return;
    if (msg->body != NULL)
        free(msg->body);
    if (msg->header.method != NULL)
        free(msg->header.method);
    if (msg->header.version != NULL)
        free(msg->header.version);
    if (msg->header.url != NULL)
        free(msg->header.url);
    for (int i = 0; i < msg->header.field_cnt; i++) {
        free(msg->header.fields[i].key);
        free(msg->header.fields[i].value);
    }
    free(msg->header.fields);
}








