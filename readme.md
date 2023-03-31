# UCAS 计算机网络大作业——HTTP服务器

## 快速开始使用
### 1 初始化信息
初始化一个http_server_info，指定开启服务的类型，指定监听端口。
```c
struct http_server_info info;
init_http_server_info(&info);
info.server_info.scheme = HTTP | HTTPS;
info.server_info.http_listen_port = 80;
info.server_info.https_listen_port = 443;

```

### 2 绑定处理函数
使用bind_url将一个url地址和一个处理函数绑定。
```c
/* main.c*/
bind_url(&info, "/hello", sayHello);
/*sayHello.c*/
int sayHello(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_body(response, "Hello World!");
    return 200;
}
```

### 3 开启服务器
开始执行http服务器
```c
start_http_service(&info);
```

### 4 访问
访问http://127.0.0.1/hello或者https://127.0.0.1/hello，可以看到内容。


## 功能介绍

### 1 函数绑定
可以使用```bind_url```将一个url地址和处理函数绑定，函数原型为：
```c
int bind_url(struct http_server_info *info, const char *url, int(*handle)(int scheme, const struct http_req_msg *request, struct http_res_msg *response))
```
如果绑定成功，返回为0。而与一个url绑定的处理函数，函数原型为：
```c
/**
 * return ：状态码
 * scheme ：请求协议（HTTP or HTTPS）
 * http_req_msg ：请求报文
 * http_res_msg : 应答报文
 */
int handle(int scheme, const struct http_req_msg *request, struct http_res_msg *response)
```
当有客户端访问被绑定的url时，此函数将会被调用。

url的绑定支持后缀通配，例如绑定url```"/hello/*"```，则所有前缀为```/hello/```的路径都会被匹配。
```c
bind_url(&info, "/hello/*", sayHello);
```
如果客户访问一个没被绑定的地址，http服务器会返回404页面。
### 2 功能函数
这里提供几个功能函数，令消息的处理更便捷。

**set_res_*()系列函数**

可以设置http_res_msg(应答报文)的各种字段。其中包含
```c
void set_res_version(struct http_res_msg *msg, const char * version);/*设置version字段*/
void set_res_code(struct http_res_msg *msg, const char * code);/*设置状态码字段*/
void set_res_msg(struct http_res_msg *msg, const char * v_msg);/*设置短语字段*/
void set_res_field(struct http_res_msg *msg, const char *key, const char *value);/*设置一个head字段，如果存在则覆盖*/
void add_res_field(struct http_res_msg *msg, const char *key, const char *value);/*设置一个head字段，如果存在则跳过*/
void set_res_body(struct http_res_msg *msg, const char *body);/*设置字符串型body（不推荐使用）*/
void set_bin_res_body (struct http_res_msg *msg, const void *body, size_t size);/*设置二进制body，需要传入body长度，字符串也可以调用该函数设置，传入字符串长度。*/
```

**find_req_field()**
寻找req内的一个head字段key，赋值到value，返回0表示找到，返回-1表示未找到。
```c
int find_req_field(const struct http_req_msg *msg, const char *key, char *value);
```

### 3 常见功能实现

**重定向**

将所有HTTP请求都重定向至该路径的HTTPS。
```c
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
    if (scheme == HTTP) {
        return redirect_https_req(request, response);
    }
    /*handle request*/
    return 200;
}
```
**文件下载**

进行文件的下载
```c
int downloadFile(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
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
```
**区分GET POST等方法**
```c
int method_check(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    if (strcmp("GET", request->header.method) == 0) {
        return method_check_do_get(request, response);
    } 
    if (strcmp("POST", request->header.method) == 0) {
        return method_check_do_post(request, response);
    }
    return 403;
}
```
**自动分块传输**

如果handle函数返回状态码700，并在设置response短语为```Socket-Server-Chunked:文件路径```。则视作要求http分块传输文件，此时http服务器将会以分块的方式传输该文件，底层会二次处理此类请求。
```c
int video_mp4(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_msg(response, "Socket-Server-Chunked:data/video.mp4");
    set_res_field(response, "Content-Type", "video/mpeg");
    return 700;
}
```
**范围请求**

应对客户端进行范围请求，（request里面有Range字段）。
```c
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
int handle_range(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
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
```
**映射资源目录**

设置一个资源目录```dir```，允许客户端访问该目录下的所有文件。
```c
int handle_resource(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
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
```
**推送视频流**

可以使用分块传输的方式进行视频流的传输。
```c
int video(int scheme, const struct http_req_msg *request, struct http_res_msg *response) {
    set_res_msg(response, "Socket-Server-Chunked:data/video.mp4");
    set_res_field(response, "Content-Type", "video/mpeg");
    return 701;
}

/*main.c*/
bind_url(&info, "/video.mp4", video);
```
此时使用vlc打开视频串流http://localhost/video.avi，即可观看视频。

## 使用mininet测试

首先确保安装mininet，然后运行topo.py，打开h1节点的xterm，开启服务器。打开h2节点的xterm，运行test/test.py。如果向进行视频观看测试，可以在h2中打开vlc进行测试（需要安装图像界面。）

