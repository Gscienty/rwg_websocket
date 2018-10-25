#include "web/default_startup.hpp"
#include "web/not_impl_startup.hpp"
#include "web/server.hpp"
#include "websocket/websocket_startup.hpp"
#include "staticfile/staticfile_startup.hpp"

#include <iostream>

int main() {
    rwg_websocket::startup websock;
    auto websock_handle = [] (rwg_websocket::frame &frame) -> void {
        /* std::cout << frame.fin_flag() << std::endl; */
        /* std::cout << frame.mask() << std::endl; */
        /* std::cout << frame.masking_key() << std::endl; */
        /* std::cout << frame.opcode() << std::endl; */
        std::string val(frame.payload().begin(), frame.payload().end());
        std::cout << val << std::endl;

        rwg_websocket::frame resp_frame(frame.fd());

        resp_frame.fin_flag() = true;
        resp_frame.mask() = false;
        resp_frame.opcode() = rwg_websocket::op::op_text;
    };
    websock.set_handle(websock_handle);

    rwg_staticfile::startup staticfile("/static/", "./websocket/test/");
    staticfile.read_config("./staticfile/mime.conf");

    rwg_web::default_startup default_startup("/static/index.html");
    rwg_web::not_impl_startup not_impl;

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

            default_startup.run(req, res);

            if (websock.handshake(req, res)) {
                return;
            }
            if (staticfile.run(req, res)) {
                return;
            }

            not_impl.run(req, res);
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
