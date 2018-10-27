# RWG WebSocket
该框架实现简易HTTP协议及WebSocket协议。实现方式由epoll实现I/O多路复用。
服务将开启n个线程（包括主线程），  n = _SC_NPROCESSORS_CONF + 1。
主线程将用于接收客户端请求，并分配到子线程中。子线程将处理传入数据、中断等事件。

该项目作为gitter_kid子项目。

## 实现简易HTTP服务器
```
#include "web/server.hpp"
#include <string>

void http_handler(rwg_web::req&, rwg_web::res& res, std::function<void ()>) {
    res.version() = "HTTP/1.1";
    res.status_code() = 200;
    res.description() = "OK";

    std::string content = "<!DOCTYPE html>              \
                           <html>                       \
                           <head>                       \
                           <title>Test Page</title>     \
                           </head>                      \
                           <body>                       \
                           <h1>Hello World</h1>         \
                           <p>This is a test page.</p>  \
                           </body>                      \
                           </html>";

    res.header_parameters()["Content-Type"] = "plain/html";
    res.header_parameters()["Content-Length"] = std::to_string(content.size());
    res.header_parameters()["Connection"] = "keep-alive";

    res.write_header();
    res.write(content.begin(), content.end());
    res.flush();
}

int main() {
    rwg_web::server server;
    server.listen("127.0.0.1", 8088);
    server.http_handle(http_handler);
    server.start();

    return 0;
}
```
