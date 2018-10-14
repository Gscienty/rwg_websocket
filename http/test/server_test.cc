#include "server.h"
#include <iostream>

int main() {
    rwg_http::server server(16);

    server.init_thread_pool(16);
    server.init_buffer_pool(128);

    auto middleware = [] (rwg_http::session& sess) -> bool {
        sess.req().get_header();
        std::cout << sess.req().uri() << std::endl;

        return false;
    };

    server.chain().append(middleware);

    server.listen("127.0.0.1", 8088);
    std::cout << "START\n";
    server.start(1, -1);

    return 0;
}
