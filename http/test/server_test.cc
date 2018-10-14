#include "server.h"
#include <iostream>

int main() {
    rwg_http::server server(16);

    server.init_thread_pool(16);
    server.init_buffer_pool(128);

    auto middleware = [] (rwg_http::session& sess) -> bool {
        std::cout << sess.req().uri() << std::endl;

        sess.res().status_code() = 400;
        sess.res().description() = "Bad Request";
        sess.res().header_parameters()["Connection"] = "close";
        sess.res().header_parameters()["Content-Type"] = "text/html";
        std::string html = "<h1>Bad Request</h1>\r\n\r\n";
        sess.res().header_parameters()["Content-Length"] = std::to_string(html.size());

        sess.res().write_header();
        sess.res().write(html.c_str(), html.size());
        sess.res().flush();

        sess.close();
        return false;
    };

    server.chain().append(middleware);

    server.listen("127.0.0.1", 8088);
    std::cout << "START\n";
    server.start(1, -1);

    return 0;
}
