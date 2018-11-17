#include "web/default_startup.hpp"
#include "web/not_impl_startup.hpp"
#include "web/server.hpp"
#include "websocket/websocket_startup.hpp"
#include "staticfile/staticfile_startup.hpp"

#include <iostream>

int main() {
    auto websock_endpoint_factory = [] (rwg_web::req &) -> std::unique_ptr<rwg_websocket::endpoint> {
        return std::unique_ptr<rwg_websocket::endpoint>(new rwg_websocket::endpoint());
    };
    auto websock_handle = [] (rwg_websocket::endpoint &endpoint, std::function<void ()>) -> void {
        std::string val(endpoint.frame().payload().begin(), endpoint.frame().payload().end());
        std::cout << val << std::endl;
    };

    rwg_staticfile::startup staticfile("/static/", "./websocket/test/");
    staticfile.read_config("./staticfile/mime.conf");

    rwg_web::default_startup default_startup("/static/index.html");
    rwg_web::not_impl_startup not_impl;

    rwg_web::server server(10, 10, 2000);
    server.listen("0.0.0.0", 5000);

    auto web_handle = [&] (rwg_web::req& req, rwg_web::res& res, std::function<void ()> close_cb) -> void {
        std::cout << req.uri() << std::endl;

        res.header_parameters()["Connection"] = "keep-alive";
        default_startup.run(req, res);

        if (staticfile.run(req, res)) {
            std::cout << "staticfile end" << std::endl;
            return;
        }

        not_impl.run(req, res);
        close_cb();
    };

    server.http_handle(web_handle);
    server.websocket_endpoint_factory(websock_endpoint_factory);
    server.websocket_handshake_handle([] (rwg_web::req&) -> bool { return true; });
    server.websocket_frame_handle(websock_handle);

    server.start();

    return 0;
}
