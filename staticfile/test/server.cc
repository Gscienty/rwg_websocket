#include "server.hpp"
#include "staticfile_startup.hpp"
#include <iostream>

int main() {
    rwg_staticfile::startup staticfile("/static/", "./");
    staticfile.read_config("./mime.conf");

    rwg_web::server server(10, 10, 2000);
    server.listen("127.0.0.1", 8088);

    auto func = [&] (rwg_web::req& req, rwg_web::res& res) -> void {
        staticfile.run(req, res);
    };

    server.run(func);
    server.start();
    return 0;
}
