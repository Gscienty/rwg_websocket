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

void websocket_handler(rwg_websocket::frame& frame, std::function<void ()>) {
    std::string line(frame.payload().begin(), frame.payload().end());

    std::cout << line << std::endl;

    for (auto other : fd) {
        if (other == frame.fd()) { continue; }
        rwg_websocket::frame f;

        f.fd() = other;
        f.fin_flag() = true;
        f.mask() = false;
        f.opcode() = rwg_websocket::op_text;
        f.payload().assign(frame.payload().begin(), frame.payload().end());

        f.write();
    }
}

bool websocket_handshake_handler(rwg_web::req& req) {
    fd.push_back(req.fd());
    return true;
}

int main() {
    staticfile.read_config("./staticfile/mime.conf");

    rwg_web::server server;

    server.http_handle(http_handler);
    server.websocket_handshake_handle(websocket_handshake_handler);
    server.websocket_frame_handle(websocket_handler);

    server.listen("0.0.0.0", 5000);
    server.start();

    return 0;
}
