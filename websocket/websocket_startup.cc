#include "websocket/websocket_startup.hpp"

namespace rwg_websocket {

startup::startup() : _security(false) {}

bool startup::__is_websocket_handshake(rwg_web::req &req) {
    auto upgrade = req.header_parameters().find("Upgrade");
    if (upgrade == req.header_parameters().end()) {
#ifdef DEBUG
        std::cout << "not found parameter: Upgrade" << std::endl;
#endif
        return false;
    }
    if (upgrade->second != "websocket") {
#ifdef DEBUG
        std::cout << "parameter Upgrade's value not \"websocket\"" << std::endl;
#endif
        return false;
    }

    auto connection = req.header_parameters().find("Connection");
    if (connection == req.header_parameters().end()) {
#ifdef DEBUG
        std::cout << "not found parameter: Connection" << std::endl;
#endif
        return false;
    }
    if (connection->second != "Upgrade") {
#ifdef DEBUG
        std::cout << "parameter Connection's value not \"Upgrade\"" << std::endl;
#endif
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

#ifdef DEBUG
    std::cout << "websocket handshake response" << std::endl;

    std::cout << res.version() << ' ' << res.status_code() << ' ' << res.description() << std::endl;
    for (auto kv : res.header_parameters()) {
        std::cout << kv.first << ": " << kv.second << std::endl;
    }
#endif

    res.write_header();
    res.flush();

    this->_websocks.insert(std::make_pair(req.fd(), std::make_tuple(rwg_websocket::frame(), std::map<std::string, std::string>())));
    auto &c_ws = this->_websocks[req.fd()];
    std::get<1>(c_ws)["Key"] = res.header_parameters()["Sec-WebSocket-Accept"];
    std::get<1>(c_ws)["Version"] = res.header_parameters()["Sec-WebSocket-Version"];
    std::get<1>(c_ws)["URI"] = req.uri();

    if (this->_security) {
        std::get<0>(c_ws).use_security();

        std::get<0>(c_ws).fd() = req.fd();
        std::get<0>(c_ws).ssl() = req.ssl();
    }

    if (bool(this->_init)) {
        this->_init(req.fd());
    }
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
#ifdef DEBUG
    std::cout << "websocket startup" << std::endl;
#endif
    if (this->_websocks.find(fd) == this->_websocks.end()) {
        close_cb();
    }

    auto &frame = std::get<0>(this->_websocks[fd]);
    bool do_next = true;
    while (do_next) {
        rwg_websocket::frame_parse_stat remember_stat = frame.stat();

        frame.parse();

        switch (frame.stat()) {
        case rwg_websocket::fpstat_end:
            if (bool(this->_func)) {
                this->_func(frame, close_cb);
            }
            frame.reset();
            break;
        case rwg_websocket::fpstat_next:
            frame.stat() = remember_stat;
            do_next = false;
            break;
        case rwg_websocket::fpstat_err:
#ifdef DEBUG
            std::cout << "websocket parse error" << std::endl;
#endif
            do_next = false;
            close_cb();
            break;
        case rwg_websocket::fpstat_interrupt:
#ifdef DEBUG
            std::cout << "websocket interrupt" << std::endl;
#endif
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

    if (this->available() && bool(this->_handshake) && this->_handshake(req)) {
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
        this->_websocks.erase(itr);
        if (bool(this->_remove)) {
            this->_remove(fd);
        }
    }
}

bool startup::is_websocket(int fd) {
    return this->_websocks.find(fd) != this->_websocks.end();
}

void startup::frame_handle(std::function<void (rwg_websocket::frame &, std::function<void ()>)> func) {
    this->_func = func;
}

void startup::handshake_handle(std::function<bool (rwg_web::req &)> handler) {
    this->_handshake = handler;
}

void startup::init_handle(std::function<void (int)> handler) {
    this->_init = handler;
}

void startup::remove_handle(std::function<void (int)> handler) {
    this->_remove = handler;
}

bool startup::available() const {
    return bool(this->_func);
}
}
