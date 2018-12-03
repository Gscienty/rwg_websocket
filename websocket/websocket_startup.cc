#include "websocket/websocket_startup.hpp"
#include "util/debug.hpp"

namespace rwg_websocket {

int &endpoint::fd() { return this->_fd; }

rwg_websocket::frame &endpoint::frame() { return this->_frame; }

startup::startup() : _security(false) {}

bool startup::__is_websocket_handshake(rwg_web::req &req) {
    auto upgrade = req.header_parameters().find("Upgrade");
    if (upgrade == req.header_parameters().end()) {
        warn("ws_startup[%d]: not found parameter: Upgrade", req.fd());
        return false;
    }
    if (upgrade->second != "websocket") {
        warn("ws_startup[%d]: parameter Upgrade's value not \"websocket\"", req.fd());
        return false;
    }

    auto connection = req.header_parameters().find("Connection");
    if (connection == req.header_parameters().end()) {
        warn("ws_startup[%d]： not found parameter: Connection", req.fd());
        return false;
    }
    if (connection->second != "Upgrade") {
        warn("ws_startup[%d]: parameter Connection's value not \"Upgrade\"", req.fd());
        return false;
    }

    return true;
}

void startup::__accept(rwg_web::req& req, rwg_web::res& res) {
    res.status_code() = 101;
    res.version() = "HTTP/1.1";
    res.description() = "Switching Protocols";

    std::string key = req.header_parameters()["Sec-WebSocket-Key"];
    key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::basic_string<std::uint8_t> uint8_key(key.begin(), key.end());
    std::basic_string<std::uint8_t> sha1_key = rwg_util::sha1(uint8_key);

    res.header_parameters()["Upgrade"] = "websocket";
    res.header_parameters()["Connection"] = "Upgrade";
    res.header_parameters()["Sec-WebSocket-Accept"] = 
        rwg_util::base64_encode(sha1_key.c_str(), sha1_key.size());
    res.header_parameters()["Sec-WebSocket-Version"] = "13";

/* #ifdef DEBUG */
/*     std::cout << "websocket handshake response" << std::endl; */

/*     std::cout << res.version() << ' ' << res.status_code() << ' ' << res.description() << std::endl; */
/*     for (auto kv : res.header_parameters()) { */
/*         std::cout << kv.first << ": " << kv.second << std::endl; */
/*     } */
/* #endif */

    res.write_header();
    res.flush();

    std::unique_ptr<rwg_websocket::endpoint> endpoint_ptr = this->_factory(req);

    endpoint_ptr->fd() = req.fd();
    endpoint_ptr->frame().fd() = req.fd();

    if (this->_security) {
        endpoint_ptr->frame().use_security();
        endpoint_ptr->frame().ssl() = req.ssl();
    }

    if (bool(this->_init)) {
        this->_init(*endpoint_ptr);
    }

    this->_websocks[req.fd()] = std::move(endpoint_ptr);
}

void startup::__reject(rwg_web::req&, rwg_web::res& res) {
    res.status_code() = 400;
    res.version() = "HTTP/1.1";
    res.description() = "Bad Request";

    res.header_parameters()["Connection"] = "close";

    res.write_header();
    res.flush();
}

void startup::use_security(bool use) {
    this->_security = use;
}

void startup::run(int fd, std::function<void ()> close_cb) {
    info("ws_startup[%d]: enter websocket startup", fd);
    if (this->_websocks.find(fd) == this->_websocks.end()) {
        warn("ws_startup[%d]: not found online ws", fd);
        close_cb();
    }

    auto &c_ws = this->_websocks[fd];

    bool do_next = true;
    while (do_next) {
        rwg_websocket::frame_parse_stat remember_stat = c_ws->frame().stat();

        c_ws->frame().parse();

        switch (c_ws->frame().stat()) {
        case rwg_websocket::fpstat_end:
            if (bool(this->_func)) {
                this->_func(*c_ws, close_cb);
            }
            c_ws->frame().reset();
            break;
        case rwg_websocket::fpstat_next:
            c_ws->frame().stat() = remember_stat;
            do_next = false;
            break;
        case rwg_websocket::fpstat_err:
            warn("ws_startup[%d]: parse error", fd);
            do_next = false;
            close_cb();
            break;
        case rwg_websocket::fpstat_interrupt:
            warn("ws_startup[%d]: interrupt", fd);
            do_next = false;
            close_cb();
            break;
        default:
            break;
        }
    }
}

bool startup::handshake(rwg_web::req& req, rwg_web::res& res, std::function<void ()> close_cb) {
    if (!this->__is_websocket_handshake(req)) {
        return false;
    }

    if (this->available() && bool(this->_handshake) && bool(this->_factory) && this->_handshake(req)) {
        this->__accept(req, res);
    }
    else {
        this->__reject(req, res);
        close_cb();
    }

    return true;
}

/* void startup::add(int fd) { */
/*     this->remove(fd); */
/*     this->_websocks.insert(std::make_pair(fd, std::map<std::string, std::string>())); */
/* } */

void startup::remove(int fd) {
    auto itr = this->_websocks.find(fd);
    if (itr != this->_websocks.end()) {
        if (bool(this->_remove)) {
            this->_remove(*itr->second);
        }
        this->_websocks.erase(itr);
    }
}

bool startup::is_websocket(int fd) {
    return this->_websocks.find(fd) != this->_websocks.end();
}

void startup::frame_handle(std::function<void (rwg_websocket::endpoint &, std::function<void ()>)> func) {
    this->_func = func;
}

void startup::handshake_handle(std::function<bool (rwg_web::req &)> handler) {
    this->_handshake = handler;
}

void startup::init_handle(std::function<void (rwg_websocket::endpoint &)> handler) {
    this->_init = handler;
}

void startup::remove_handle(std::function<void (rwg_websocket::endpoint &)> handler) {
    this->_remove = handler;
}

void startup::endpoint_factory(std::function<std::unique_ptr<rwg_websocket::endpoint> (rwg_web::req &)> factory) {
    this->_factory = factory;
}

bool startup::available() const {
    return bool(this->_func);
}
}
