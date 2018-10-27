#include "web/default_startup.hpp"
#include "web/not_impl_startup.hpp"
#include "web/server.hpp"
#include "websocket/websocket_startup.hpp"
#include "staticfile/staticfile_startup.hpp"

#include <iostream>

int main() {
    auto websock_handle = [] (rwg_websocket::frame &frame) -> void {
        std::string val(frame.payload().begin(), frame.payload().end());
        std::cout << val << std::endl;
    };

    rwg_staticfile::startup staticfile("/static/", "./websocket/test/");
    staticfile.read_config("./staticfile/mime.conf");

    rwg_web::default_startup default_startup("/static/index.html");
    rwg_web::not_impl_startup not_impl;

    rwg_web::server server(10, 10, 2000);
    server.listen("0.0.0.0", 5000);

    auto web_handle = [&] (rwg_web::req& req, rwg_web::res& res) -> void {
        default_startup.run(req, res);

        if (staticfile.run(req, res)) {
            return;
        }

        not_impl.run(req, res);
    };

    server.http_handle(web_handle);
    server.websocket_heandle(websock_handle);

    server.start();

    return 0;
}
