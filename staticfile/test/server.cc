#include "server.h"
#include "startup.h"
#include <iostream>

int main() {
    rwg_http::server server(16);

    server.init_buffer_pool(4096);
    server.init_thread_pool(16);

    rwg_staticfile::startup staticfile("/static/", "./test/");
    staticfile.read_config("./mime.conf");

    auto func = [] (rwg_http::session&) -> bool {
        std::cout << "ACC" << std::endl;
        return true;
    };

    server.chain().append(func);

    server.chain().append_middleware(staticfile);

    server.listen("127.0.0.1", 8088);
    std::cout << "RUNNING\n";
    server.start(16, -1);

    return 0;
}
