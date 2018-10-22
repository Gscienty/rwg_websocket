#include "web/server.hpp"
#include "websocket/websocket_startup.hpp"
#include "staticfile/staticfile_startup.hpp"

#include <iostream>

int main() {

    rwg_websocket::startup websock;
    auto websock_handle = [] (rwg_websocket::frame &) -> void {
        std::cout << "HELLO WORLD" << std::endl;
    };
    websock.set_handle(websock_handle);

    rwg_staticfile::startup staticfile("/static/", "./test/");
    staticfile.read_config("../staticfile/mime.conf");

    rwg_web::server server(10, 10, 2000);
    server.listen("0.0.0.0", 5000);

    auto web_handle = [&] (rwg_web::req& req, rwg_web::res& res) -> void {
        if (websock.is_websocket(req.fd())) {
            websock.run(req.fd());
        }
        else {
            req.alloc_buf(512);
            res.alloc_buf(512);
            req.parse();

            if (websock.handshake(req, res)) {
                return;
            }
            if (staticfile.run(req, res)) {
                return;
            }
        }
    };

    auto close_handle = [&] (int fd) -> void {
        websock.remove(fd);
    };


    server.run(web_handle);
    server.close_handle(close_handle);

    server.start();

    return 0;
}
