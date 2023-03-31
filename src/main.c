#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "socket_server.h"
#include "http_server.h"
#include "utils/http_msg.h"

#include "handle_func.h"

void run() {
    srand(time(NULL));


    struct http_server_info info;
    init_http_server_info(&info);
    
    info.server_info.scheme = HTTP | HTTPS;
    // info.server_info.http_listen_port = 8080 + rand() % 10;
    // info.server_info.https_listen_port = 8090 + rand() % 10;
    info.server_info.http_listen_port = 80;
    info.server_info.https_listen_port = 443;
    bind_url(&info, "/hello", say_hello);
    bind_url(&info, "/goodbye", say_goodbye);
    bind_url(&info, "/input/*", say_input);
    bind_url(&info, "/say_json", say_json);
    bind_url(&info, "/download", download_file);
    bind_url(&info, "/downloadB", download_big_file);
    bind_url(&info, "/check_method", method_check);
    bind_url(&info, "/video.avi", video);
    bind_url(&info, "/redirect", redirect);
    bind_url(&info, "/video.mp4", video_mp4);

    bind_url(&info, "/index.html", for_test_py_index);
    bind_url(&info, "/dir/*", for_test_py_dir);
    start_http_service(&info);
}


int main(int argc, char * argv[]) {
    run();
    return 0;
}