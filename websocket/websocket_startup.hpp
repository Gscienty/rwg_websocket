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
    std::function<void (rwg_websocket::frame& frame, std::function<void ()>)> _func;
    std::function<bool (rwg_web::req&)> _handshake;
    rwg_websocket::frame _frame;

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

    void __accept(rwg_web::req& req, rwg_web::res& res) {
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

        this->_websocks.insert(std::make_pair(req.fd(), std::map<std::string, std::string>()));
        this->_websocks[req.fd()]["Key"] = res.header_parameters()["Sec-WebSocket-Accept"];
        this->_websocks[req.fd()]["Version"] = res.header_parameters()["Sec-WebSocket-Version"];
        this->_websocks[req.fd()]["URI"] = req.uri();
    }

    void __reject(rwg_web::req&, rwg_web::res& res) {
        res.status_code() = 400;
        res.version() = "HTTP/1.1";
        res.description() = "Bad Request";
        
        res.header_parameters()["Connection"] = "close";

        res.write_header();
        res.flush();
    }

public:
    void run(int fd, std::function<void ()> close_cb) {
#ifdef DEBUG
        std::cout << "websocket startup" << std::endl;
#endif
        if (this->_frame.fd() != fd) {
#ifdef DEBUG
            std::cout << "websocket frame origin fd differ current fd" << std::endl;
#endif
            this->_frame.fd() = fd;
        }

        bool do_next = true;
        while (do_next) {
            rwg_websocket::frame_parse_stat remember_stat = this->_frame.stat();

            this->_frame.parse();

            switch (this->_frame.stat()) {
            case rwg_websocket::fpstat_end:
                if (bool(this->_func)) {
                    this->_func(this->_frame, close_cb);
                }
                this->_frame.reset();
                break;
            case rwg_websocket::fpstat_next:
                this->_frame.stat() = remember_stat;
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

    bool handshake(rwg_web::req& req, rwg_web::res& res, std::function<void ()> close_cb) {
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

    void handle(std::function<void (rwg_websocket::frame&, std::function<void ()>)> func) {
        this->_func = func;
    }

    void handshake_handle(std::function<bool (rwg_web::req&)> handler) {
        this->_handshake = handler;
    }

    bool available() const {
        return bool(this->_func);
    }

};

}

#endif
