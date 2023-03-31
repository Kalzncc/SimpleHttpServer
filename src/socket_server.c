#include "socket_server.h"

#include "utils/debug.h"



int handle(struct req_handle_info *handle_info, char *buf, size_t n, int way, SSL *ssl) {
    char *res_buf = (char *)malloc(MESSAGE_MAX_BUFF * sizeof(char));
    

    int new_n = 1;
    for (int i = 1; i < n; i++) {
        if (buf[i-1] == 13 && buf[i] == 10) {
            buf[new_n-1] = 10;
            continue;
        }
        buf[new_n++] = buf[i];
    }
    n = new_n;
    buf[n] = '\0';

    struct http_req_msg req_msg;
    parse_http_req_msg(buf, n, &req_msg);

    struct http_res_msg res_msg;
    memset(&res_msg, 0, sizeof(struct http_res_msg));
    
    res_msg.header.field_cnt = 0;
    set_res_code(&res_msg, "407");
    set_res_version(&res_msg, req_msg.header.version);
    set_res_msg(&res_msg, "Do not handle.");
    res_msg.body = NULL;

    _print_http_req_msg(&req_msg);
    handle_info->info->handle_req(way, &req_msg, &res_msg, handle_info->info->super_info);

    if (strcmp(res_msg.header.code, "700") == 0) {
    /*
        The status code 700 is defined by myself. 
        It's means handle function need chunked transfer a file.
    */
        set_res_code(&res_msg, "200");
        char *path = res_msg.header.msg+22;
        set_res_msg(&res_msg, "OK");
        set_res_field(&res_msg, "Transfer-Encoding", "chunked");
        res_msg.body_len = 0;
        res_msg.body = NULL;
        size_t res_len = stringify_http_res_msg(res_buf, &res_msg);
        int send_len = 0;
        if (way == HTTP) {
            send_len = send(handle_info->connfd, res_buf, res_len, 0);
        } else {
            send_len = SSL_write(ssl, res_buf, res_len);
        }
        if (send_len < 0) 
            goto err;
        
        FILE *fp = fopen(path, "rb");
        char buf[SSL_CHUNKED_MAX_BUF];
        size_t len; 
        while((len = fread(buf, 1, SSL_CHUNKED_MAX_BUF, fp)) > 0) {
            int p = sprintf(res_buf, "%X\r\n", (int)len);
            memcpy(res_buf+p, buf, len);
            *(res_buf+p+len) = '\r';
            *(res_buf+p+len+1) = '\n';
            if (way == HTTP) {
                send_len = send(handle_info->connfd, res_buf, len + p + 2, 0);
            } else {
                send_len = SSL_write(ssl, res_buf, len + p + 2);
            }
            if (send_len < 0) 
                goto err;
        }
        sprintf(res_buf, "0\r\n\r\n");
        if (way == HTTP) {
            send_len = send(handle_info->connfd, res_buf, 5, 0);
        } else {
            send_len = SSL_write(ssl, res_buf, 5);
        }
        if (send_len < 0)
            goto err;
        fclose(fp);
    } else if (strcmp(res_msg.header.code, "701") == 0) {
        char path_buf[SHORT_BUF];
        size_t file_seek = 0;
        find_chunked_range_info(&res_msg, path_buf, &file_seek);
        
        set_res_field(&res_msg, "Accept-Ranges", "bytes");


        FILE *fp = fopen(path_buf, "rb");
        fseek(fp, 0, SEEK_END);
        size_t file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        _printf("file_size <%ld>\n", file_size);
        _printf("file_path %s\n", path_buf);
        sprintf(res_buf, "%ld", file_size - file_seek);
        set_res_field(&res_msg, "Content-Length", res_buf);
        fseek(fp, file_seek, SEEK_SET);
        

        if (file_seek == 0) {
            set_res_code(&res_msg, "200");
            set_res_msg(&res_msg, "Last-Modified");
        } else {
            set_res_code(&res_msg, "206");
            set_res_msg(&res_msg, "Last-Modified");
            sprintf(res_buf, "bytes %ld-%ld/%ld", file_seek, file_size-1, file_size);
            set_res_field(&res_msg, "Content-Range", res_buf);
        }


        int first_flag = 1;
        size_t done_len = 0;
        while(1) {
            if (first_flag) {
                first_flag = 0;
                size_t res_len = fread(res_buf, 1, SINGLE_MAX_BUF >> 1, fp);
                if (res_len == 0) break;
                done_len += res_len;
                set_bin_res_body(&res_msg, res_buf, res_len);
                _print_http_res_msg(&res_msg);
                res_len = stringify_http_res_msg(res_buf, &res_msg);
                int send_len;
                if (way == HTTP) {
                    send_len = send(handle_info->connfd, res_buf, res_len, 0);
                } else {
                    send_len = SSL_write(ssl, res_buf, res_len);
                }
                if (send_len < 0) 
                    goto err;
            } else {
                size_t res_len = fread(res_buf, 1, SINGLE_MAX_BUF, fp);
                if (res_len == 0) break;
                done_len += res_len;
                int send_len;
                
                if (way == HTTP) {
                    send_len = send(handle_info->connfd, res_buf, res_len, 0);
                } else {
                    send_len = SSL_write(ssl, res_buf, res_len);
                }
                if (send_len < 0) 
                    goto err;
            }
        }
        printf("<<done>> %ld\n", done_len);
        fclose(fp);
    } else {
        _print_http_res_msg(&res_msg); 
        size_t res_len = stringify_http_res_msg(res_buf, &res_msg);

        if (way == HTTP) {
            send(handle_info->connfd, res_buf, res_len, 0);
        } else {
            SSL_write(ssl, res_buf, res_len);
        }
    }
    
    free(res_buf);
    free_req_msg(&req_msg);
    free_res_msg(&res_msg);
    return 0;


err:
    printf("RST:::::::: %d\n", handle_info->connfd);
    free(res_buf);
    free_req_msg(&req_msg);
    free_res_msg(&res_msg);
    return 1;
}



void* handle_http(void *v_info) {

    struct req_handle_info *handle_info = (struct req_handle_info *) v_info;
    
    _printf("connfd : %d\n", handle_info->connfd);

    char *buf = (char *)malloc(MESSAGE_MAX_BUFF * sizeof(char));
    

    while(1) {
        size_t n = recv(handle_info->connfd, buf, MESSAGE_MAX_BUFF, 0);    
        buf[n] = '\0';
        if (n <= 0) {
            if (errno != 0)
                fprintf(stderr, "rev error : %s(errno: %d)\n", strerror(errno), errno);
            else 
                continue;
            shutdown(handle_info->connfd, SHUT_RDWR);
            free(buf);
            return NULL;
        }
        if (handle(handle_info, buf, n, HTTP, NULL)) {
            break;
        }
        break;
    }


    shutdown(handle_info->connfd, SHUT_RDWR);
    free(buf);
    return NULL;

}


void* handle_https(void *v_info) {
    struct req_handle_info *handle_info = (struct req_handle_info *) v_info;

    _printf("connfd : %d\n", handle_info->connfd);

    SSL *ssl = SSL_new(handle_info->ctx);
    SSL_set_fd(ssl, handle_info->connfd);
    char *buf = (char *)malloc(MESSAGE_MAX_BUFF * sizeof(char));
    
    if (SSL_accept(ssl) == -1) {
        fprintf(stderr, "accpet ssl socket error : %s(errno: %d)\n", strerror(errno), errno);
        return NULL;
    }

    while(1) {
        size_t n = SSL_read(ssl, buf, MESSAGE_MAX_BUFF);
        buf[n] = '\0';
        if (n <= 0) {
            if (errno != 0)
                fprintf(stderr, "recv ssl socket error : %s(errno: %d)\n", strerror(errno), errno);
            else 
                continue;
            free(buf);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            shutdown(handle_info->connfd, SHUT_RDWR);
            return NULL;
        }
        if (handle(handle_info, buf, n, HTTPS, ssl)) {
            break;
        }
        break;
    }

    free(buf);
    SSL_shutdown(ssl);
    SSL_free(ssl);
    shutdown(handle_info->connfd, SHUT_RDWR);
    return NULL;
}


void* listen_http(void *v_info) {
    
    struct server_info *info = (struct server_info *)v_info;
    
    int listenfd, connfd;
    struct sockaddr_in servaddr;

    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) ==-1 ) {
        fprintf(stderr, "create socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(info->http_listen_port);

    if ( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr) ) == -1 ) {
        fprintf(stderr, "bind socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if ( listen(listenfd, 10) ==-1 ) {
        fprintf(stderr, "listen socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    
    printf("Listen Start (HTTP) on  %d http://127.0.0.1:%d\n", (int)info->http_listen_port, (int)info->http_listen_port);
    while(1) {
        
        if ( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1 ) {
            fprintf(stderr, "accpet socket error : %s(errno: %d)\n", strerror(errno), errno);
            exit(0);
        }
        
        pthread_t handle_thread = 0;
        struct req_handle_info handle_info;
        handle_info.info = info;
        handle_info.connfd = connfd;
        pthread_create(&handle_thread, NULL, handle_http, &handle_info);

    }
    close(listenfd);
}

void* listen_https(void *v_info) {
    struct server_info *info = (struct server_info *)v_info;
    int listenfd, connfd;
    struct sockaddr_in servaddr;


    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    
    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);

    if (ctx == NULL) {
        fprintf(stderr, "create ssl error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    
    if (SSL_CTX_use_certificate_file(ctx, "./keys/cnlab.cert", SSL_FILETYPE_PEM) < 0) {
        fprintf(stderr, "set ssl cert error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "./keys/cnlab.prikey", SSL_FILETYPE_PEM) < 0) {
        fprintf(stderr, "set ssl private key error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "ssl private key error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    
    if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) ==-1 ) {
        fprintf(stderr, "create socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(info->https_listen_port);

    if ( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr) ) == -1 ) {
        fprintf(stderr, "bind socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    if ( listen(listenfd, 10) ==-1 ) {
        fprintf(stderr, "listen socket error : %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }

    printf("Listen Start (HTTPS) on %d https://127.0.0.1:%d\n", (int)info->https_listen_port, (int)info->https_listen_port);
    while(1) {
        if ( (connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1 ) {
            fprintf(stderr, "accpet socket error : %s(errno: %d)\n", strerror(errno), errno);
            exit(0);
        }

        pthread_t handle_thread = 0;
        struct req_handle_info handle_info;
        handle_info.info = info;
        handle_info.connfd = connfd;
        handle_info.ctx = ctx;
        pthread_create(&handle_thread, NULL, handle_https, &handle_info);
    }
    close(listenfd);


}





void run_http_server(struct server_info * info) {

    signal(SIGPIPE, SIG_IGN);
    /*
        write, send or read a socket stream will raise a SIGPIPE, After Client send RST.
        If Do not handle it, program will crash.
    */


    pthread_t pt_t[2] = { 0 };
    if (info->scheme & HTTP) {
        pthread_create(&pt_t[0],NULL,listen_http,info);
    }
    if (info->scheme & HTTPS) {
        pthread_create(&pt_t[1],NULL,listen_https,info);
    }
    if (pt_t[0] != 0)
        pthread_join(pt_t[0], NULL);
    if (pt_t[1] != 0)
        pthread_join(pt_t[1], NULL);
}