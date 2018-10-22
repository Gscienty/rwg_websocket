#include "web/req.hpp"
#include "web/res.hpp"
#include "websocket/websocket_startup.hpp"

#include <iostream>

int main() {

    rwg_websocket::startup websock;

    rwg_web::req req(0);
    rwg_web::res res(1);

    req.header_parameters()["Upgrade"] = "websocket";
    req.header_parameters()["Connection"] = "Upgrade";
    req.header_parameters()["Sec-WebSocket-Key"] = "sN9cRrP/n9NdMgdcy2VJFQ==";
    req.header_parameters()["Sec-WebSocket-Version"] = "13";

    websock.handshake(req, res);

    std::cout << res.header_parameters()["Sec-WebSocket-Accept"];

    return 0;
}
