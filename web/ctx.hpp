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
    std::function<void (int)>_close_cb;
    bool _close_flag;

    void __close() {
        this->_close_flag = true;
    }
public:
    ctx(std::function<void (rwg_web::req&, rwg_web::res&, std::function<void ()>)> http_handler,
        rwg_websocket::startup &websocket,
        std::function<void (int)> close_cb)
        : _http_ctx(websocket, http_handler) 
        , _websocket(websocket)
        , _close_cb(close_cb)
        , _close_flag(false) {}

    virtual ~ctx() {
    }

    virtual void in_event() override {
        if (this->_websocket.is_websocket(this->ep_event().data.fd)) {
            this->_websocket.run(this->ep_event().data.fd, std::bind(&rwg_web::ctx::__close, this));
        }
        else {
            this->_http_ctx.run(this->ep_event().data.fd, std::bind(&rwg_web::ctx::__close, this));
        }

        if (this->_close_flag) {
            this->close();
        }
    }

    void close() {
        this->_close_cb(this->ep_event().data.fd);
    }
};

}

#endif
