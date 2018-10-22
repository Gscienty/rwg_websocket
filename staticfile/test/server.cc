#include "web/server.hpp"
#include "staticfile/staticfile_startup.hpp"
#include <iostream>

int main() {
    rwg_staticfile::startup staticfile("/static/", "./");
    staticfile.read_config("./mime.conf");

    rwg_web::server server(10, 10, 2000);
    server.listen("0.0.0.0", 5000);

    auto func = [&] (rwg_web::req& req, rwg_web::res& res) -> void {
        req.alloc_buf(512);
        res.alloc_buf(512);
        req.parse();
        staticfile.run(req, res);
    };

    server.run(func);
    server.start();
    return 0;
}
