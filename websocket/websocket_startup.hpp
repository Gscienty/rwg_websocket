#ifndef _RWG_WEBSOCKET_STARTUP_
#define _RWG_WEBSOCKET_STARTUP_

#include "web/req.hpp"
#include "web/res.hpp"
#include "util/base64.hpp"
#include "util/sha1.hpp"
#include "websocket/frame.hpp"

#include <map>
#include <string>
#include <functional>

#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_websocket {

class startup {
private:
    std::map<int, std::map<std::string, std::string>> _websocks;
    std::function<void (rwg_websocket::frame& frame)> _func;

    bool __is_websocket_handshake(rwg_web::req& req) {
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

public:
    void run(int fd) {
#ifdef DEBUG
        std::cout << "websocket startup" << std::endl;
#endif
        rwg_websocket::frame frame(fd);
        frame.parse();

        if (bool(this->_func)) {
            this->_func(frame);
        }
    }

    bool handshake(rwg_web::req& req, rwg_web::res& res) {
        if (!this->__is_websocket_handshake(req)) {
            return false;
        }
        
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
        /* res.header_parameters()["Sec-WebSocket-Protocol"] = req.header_parameters()["Sec-WebSocket-Protocol"]; */

#ifdef DEBUG
        std::cout << "websocket handshake response" << std::endl;

        std::cout << res.version() << ' ' << res.status_code() << ' ' << res.description() << std::endl;
        for (auto kv : res.header_parameters()) {
            std::cout << kv.first << ": " << kv.second << std::endl;
        }
#endif

        res.write_header();
        res.flush();

        this->_websocks.insert(std::make_pair(req.fd(), std::map<std::string, std::string>()));
        this->_websocks[req.fd()]["Key"] = res.header_parameters()["Sec-WebSocket-Accept"];
        this->_websocks[req.fd()]["Version"] = res.header_parameters()["Sec-WebSocket-Version"];
        this->_websocks[req.fd()]["URI"] = req.uri();

        return true;
    }

    void add(int fd) {
        this->remove(fd);
        this->_websocks.insert(std::make_pair(fd, std::map<std::string, std::string>()));
    }

    void remove(int fd) {
        auto itr = this->_websocks.find(fd);
        if (itr != this->_websocks.end()) {
            this->_websocks.erase(itr);
        }
    }

    bool is_websocket(int fd) {
        return this->_websocks.find(fd) != this->_websocks.end();
    }

    void set_handle(std::function<void (rwg_websocket::frame&)> func) {
        this->_func = func;
    }

};

}

#endif
