#ifndef _RWG_WEB_CTX_
#define _RWG_WEB_CTX_

#include "web/abstract_in_event.hpp"
#include "web/http_ctx.hpp"
#include "websocket/websocket_startup.hpp"
#include <functional>

namespace rwg_web {

class ctx : public rwg_web::abstract_in_event {
private:
    int _epfd;
    rwg_web::http_ctx _http_ctx;
    rwg_websocket::startup &_websocket;
    std::function<void (::epoll_event&)> _close_cb;

public:
    ctx(std::function<void (rwg_web::req&, rwg_web::res&)> http_handler,
        rwg_websocket::startup &websocket,
        std::function<void (::epoll_event&)> close_cb)
        : _http_ctx(websocket, http_handler, std::bind(&rwg_web::ctx::close, this)) 
        , _websocket(websocket)
        , _close_cb(close_cb) {}

    virtual ~ctx() {

    }

    virtual void in_event(int fd) override {
        if (this->_websocket.is_websocket(fd)) {
            this->_websocket.run(fd);
        }
        else {
            this->_http_ctx.execute(fd);
        }
    }

    void close() {
        this->_close_cb(this->ep_event());
    }
};

}

#endif
