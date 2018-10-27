#include "web/server.hpp"
#include "staticfile/staticfile_startup.hpp"
#include <iostream>

int main() {
    rwg_staticfile::startup staticfile("/static/", "staticfile/test/");
    staticfile.read_config("staticfile/mime.conf");

    rwg_web::server server(10, 10, 2000);
    server.listen("0.0.0.0", 5000);

    auto func = [&] (rwg_web::req& req, rwg_web::res& res, std::function<void ()>) -> void {
        std::cout << "staticfile" << std::endl;
        staticfile.run(req, res);
    };

    server.http_handle(func);
    server.start();
    return 0;
}
