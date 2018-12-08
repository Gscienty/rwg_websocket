#include "web/server.hpp"
#include "web/default_startup.hpp"
#include "web/not_impl_startup.hpp"
#include "staticfile/staticfile_startup.hpp"
#include "websocket/websocket_startup.hpp"
#include <vector>
#include <iostream>

rwg_web::default_startup default_startup("/static/index.html");
rwg_web::not_impl_startup not_impl;
rwg_staticfile::startup staticfile("/static/", "./example/gobang/");
rwg_websocket::startup websocket;

std::vector<int> fd;

void http_handler(rwg_web::req& req, rwg_web::res& res, std::function<void ()>) {
    default_startup.run(req, res);

    if (staticfile.run(req, res)) {
        return;
    }

    not_impl.run(req, res);
}

void websocket_handler(rwg_websocket::endpoint& endpoint, std::function<void ()>) {
    std::string line(endpoint.frame().payload().begin(), endpoint.frame().payload().end());

    std::cout << line << std::endl;

    for (auto other : fd) {
        if (other == endpoint.fd()) { continue; }
        rwg_websocket::frame f;

        f.fd() = other;
        f.fin_flag() = true;
        f.mask() = false;
        f.opcode() = rwg_websocket::op_text;
        f.payload().assign(endpoint.frame().payload().begin(), endpoint.frame().payload().end());

        f.write();
    }
}

bool websocket_handshake_handler(rwg_web::req& req) {
    fd.push_back(req.fd());
    return true;
}

std::unique_ptr<rwg_websocket::endpoint> websocket_endpoint_factory(rwg_web::req &) {
    return std::unique_ptr<rwg_websocket::endpoint>(new rwg_websocket::endpoint());
}

int main() {
    staticfile.read_config("./staticfile/mime.conf");

    rwg_web::server server;

    server.http_handle(http_handler);
    server.websocket().endpoint_factory(websocket_endpoint_factory);
    server.websocket().handshake_handle(websocket_handshake_handler);
    server.websocket().frame_handle(websocket_handler);

    server.listen("0.0.0.0", 5000);
    server.start();

    return 0;
}
