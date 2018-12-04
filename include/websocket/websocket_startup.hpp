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
#include <openssl/ssl.h>
#include <memory>

#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_websocket {

class endpoint {
private:
    int _fd;
    rwg_websocket::frame _frame;
public:
    virtual ~endpoint() {};

    int &fd();
    rwg_websocket::frame &frame();
};

class startup {
private:
    std::map<int, std::unique_ptr<rwg_websocket::endpoint>> _websocks;
    std::function<void (rwg_websocket::endpoint &, std::function<void ()>)> _func;
    std::function<bool (rwg_web::req&)> _handshake;
    std::function<void (rwg_websocket::endpoint& endpoint)> _init;
    std::function<void (rwg_websocket::endpoint& endpoint)> _remove;
    std::function<std::unique_ptr<rwg_websocket::endpoint> (rwg_web::req &)> _factory;
    bool _security;

    bool __is_websocket_handshake(rwg_web::req &);

    void __accept(rwg_web::req &, rwg_web::res &);
    void __reject(rwg_web::req &, rwg_web::res &);
public:
    startup();
    void use_security(bool use = true);
    void run(int, std::function<void ()>);
    bool handshake(rwg_web::req &, rwg_web::res &, std::function<void ()>);
    void remove(int);
    bool is_websocket(int);
    void frame_handle(std::function<void (rwg_websocket::endpoint &, std::function<void ()>)>);
    void handshake_handle(std::function<bool (rwg_web::req &)>);
    void init_handle(std::function<void (rwg_websocket::endpoint &)>);
    void remove_handle(std::function<void (rwg_websocket::endpoint &)>);
    void endpoint_factory(std::function<std::unique_ptr<rwg_websocket::endpoint> (rwg_web::req &)>);
    bool available() const;

};

}

#endif
