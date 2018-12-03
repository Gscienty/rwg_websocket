#include "web/http_ctx.hpp"
#include "util/debug.hpp"

namespace rwg_web {

http_ctx::http_ctx(int fd,
                   rwg_websocket::startup &websocket,
                   std::function<void (rwg_web::req &req, rwg_web::res &, std::function<void ()>)> handler)
    : _fd(fd)
    , _websocket(websocket)
    , _security(false)
    , _http_handler(handler) {
    this->_req.fd() = fd;
    this->_res.fd() = fd;
}

http_ctx::~http_ctx() {}

rwg_web::req &http_ctx::req() { return this->_req; }

rwg_web::res &http_ctx::res() { return this->_res; }

void http_ctx::use_security(SSL *&ssl, bool use) {
    this->_security = use;
    this->_req.use_security(ssl, use);
    this->_res.use_security(ssl, use);
}

void http_ctx::run(std::function<void ()> close_cb) {
        info("http_ctx[%d]: session received message", this->_fd);
        if (bool(this->_http_handler)) {
            info("http_ctx[%d]: parsing", this->_fd);
            this->_req.alloc_buf(512);
            this->_req.parse();
            if (this->_req.stat() == rwg_web::req_stat::req_stat_err ||
                this->_req.stat() == rwg_web::req_stat::req_stat_interrupt) {
                close_cb();
            }
            else if (this->_req.stat() == rwg_web::req_stat::req_stat_end) {
                info("http_ctx[%d]: run handle", this->_fd);
                if (!this->_websocket.handshake(this->_req, this->_res, close_cb)) {
                    info("http_ctx[%d]: normal http", this->_fd);
                    this->_http_handler(this->_req, this->_res, close_cb);
                }
#ifdef DEBUG
                else {
                    info("http_ctx[%d]: web socket handshake", this->_fd);
                }
#endif

                this->_req.reset();
                this->_res.reset();
            }
            this->_req.free_buf();
        }
    }

}
