#include "web/ctx.hpp"
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_web {

ctx::ctx(int fd,
         std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)> http_handler,
         rwg_websocket::startup &websocket,
         std::function<void (int)> close_cb)
    : _security(false)
    , _sec_inited(false)
    , _ssl(NULL)
    , _http_ctx(fd, websocket, http_handler)
    , _websocket(websocket)
    , _close_cb(close_cb)
    , _close_flag(false) {

    this->ep_event().data.fd = fd;
}

ctx::~ctx() {
    if (this->_ssl != nullptr) {
        SSL_free(this->_ssl);
    }
}

void ctx::__close() {
    this->_close_flag = true;
}

void ctx::in_event() {
    if (this->_security && !this->_sec_inited) {
#ifdef DEBUG
        std::cout << "tls start handshake" << std::endl;
#endif
        int ret = SSL_do_handshake(this->_ssl);
        switch (ret) {
        case 1:
            this->_sec_inited = true;
#ifdef DEBUG
            std::cout << "tls handshake completed" << std::endl;
#endif
            break;
        case 0:
            this->_close_flag = true;
            break;
        case -1:
            switch (SSL_get_error(this->_ssl, ret)) {
            case SSL_ERROR_WANT_READ:
                break;
            default:
                this->_close_flag = true;
                break;
            }
            break;
        default:
            this->_close_flag = true;
            break;
        }
    }
    if (this->_close_flag) {
        this->close();
        return;
    }
    if (this->_security && !this->_sec_inited) {
#ifdef DEBUG
        std::cout << "tls handshake need next" << std::endl;
#endif
        return;
    }

    if (this->_websocket.is_websocket(this->ep_event().data.fd)) {
        this->_websocket.run(this->ep_event().data.fd, std::bind(&rwg_web::ctx::__close, this));
    }
    else {
        this->_http_ctx.run(std::bind(&rwg_web::ctx::__close, this));
    }

    if (this->_close_flag) {
        this->close();
    }
}

bool ctx::use_security(SSL_CTX *ctx) {
    this->_security = true;
    this->_ssl = SSL_new(ctx);
    if (SSL_set_fd(this->_ssl, this->ep_event().data.fd) != 1) {
#ifdef DEBUG
        std::cout << "SSL_CTX_set_fd: FAILED" << std::endl;
#endif
    }
    SSL_set_accept_state(this->_ssl);

    this->_http_ctx.use_security(this->_ssl);
    return true;
}

void ctx::close() {
    if (this->_security) {
        if (this->_ssl != NULL) {
            SSL_free(this->_ssl);
            this->_ssl = NULL;
        }
    }

    this->_close_cb(this->ep_event().data.fd);
}

}
