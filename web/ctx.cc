#include "web/ctx.hpp"

namespace rwg_web {

ctx::ctx(std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)> http_handler,
         rwg_websocket::startup &websocket,
         std::function<void (int)> close_cb)
    : _http_ctx(websocket, http_handler)
    , _websocket(websocket)
    , _close_cb(close_cb)
    , _close_flag(false) {}

ctx::~ctx() {}

void ctx::__close() {
    this->_close_flag = true;
}

void ctx::in_event() {
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

void ctx::close() {
    this->_close_cb(this->ep_event().data.fd);
}

}
