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

#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_websocket {

class startup {
private:
    std::map<int, std::tuple<rwg_websocket::frame, std::map<std::string, std::string>>> _websocks;
    std::function<void (rwg_websocket::frame& frame, std::function<void ()>)> _func;
    std::function<bool (rwg_web::req&)> _handshake;
    std::function<void (int)> _init;
    std::function<void (int)> _remove;
    bool _security;

    bool __is_websocket_handshake(rwg_web::req &);

    void __accept(rwg_web::req &, rwg_web::res &);
    void __reject(rwg_web::req &, rwg_web::res &);
public:
    startup();
    void use_security(bool use = true);
    void run(int, std::function<void ()>);
    bool handshake(rwg_web::req &, rwg_web::res &, std::function<void ()>);
    /* void add(int); */
    void remove(int);
    bool is_websocket(int);
    void frame_handle(std::function<void (rwg_websocket::frame &, std::function<void ()>)>);
    void handshake_handle(std::function<bool (rwg_web::req &)>);
    void init_handle(std::function<void (int)>);
    void remove_handle(std::function<void (int)>);
    bool available() const;

};

}

#endif
