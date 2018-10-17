#include "server.h"
#include "staticfile_startup.h"
#include <iostream>

int main() {
    rwg_http::server server(1);

    server.init_buffer_pool(128);
    server.init_thread_pool(2);

    rwg_staticfile::startup staticfile("/static/", "./test/");
    staticfile.read_config("./mime.conf");

    auto func = [] (rwg_http::session&) -> bool {
        std::cout << "ACC" << std::endl;
        return true;
    };

    auto func_end = [] (rwg_http::session& session) -> bool {
        session.close();
        std::cout << "CLOSE\n";
        return false;
    };

    server.chain().append(func);
    server.chain().append_middleware(staticfile);
    server.chain().append(func_end);

    server.listen("0.0.0.0", 5000);
#ifdef DEBUG
    std::cout << "RUNNING\n";
#endif
    server.start(16, -1);

    return 0;
}
