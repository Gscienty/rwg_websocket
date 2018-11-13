#include "web/http_ctx.hpp"
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_web {

http_ctx::http_ctx(rwg_websocket::startup &websocket,
               std::function<void (rwg_web::req &req, rwg_web::res &, std::function<void ()>)> handler)
    : _websocket(websocket)
    , _http_handler(handler) {}

http_ctx::~http_ctx() {}

rwg_web::req &http_ctx::req() { return this->_req; }

rwg_web::res &http_ctx::res() { return this->_res; }

void http_ctx::run(int fd, std::function<void ()> close_cb) {
#ifdef DEBUG
        std::cout << "session received message" << std::endl;
#endif
        this->_req.fd() = fd;
        this->_res.fd() = fd;
        if (bool(this->_http_handler)) {
#ifdef DEBUG
            std::cout << "execute start http_ctx init (parse)" << std::endl;
#endif
            this->_req.alloc_buf(512);
            this->_req.parse();
            if (this->_req.stat() == rwg_web::req_stat::req_stat_err ||
                this->_req.stat() == rwg_web::req_stat::req_stat_interrupt) {
                close_cb();
            }
            else if (this->_req.stat() == rwg_web::req_stat::req_stat_end) {
#ifdef DEBUG
                std::cout << "execute start http_ctx run handle" << std::endl;
#endif
                this->_res.alloc_buf(512);
                if (!this->_websocket.handshake(this->_req, this->_res, close_cb)) {
#ifdef DEBUG
                    std::cout << "normal http" << std::endl;
#endif
                    this->_http_handler(this->_req, this->_res, close_cb);
                }
                this->_res.free_buf();

                this->_req.reset();
                this->_res.reset();
            }
            this->_req.free_buf();
        }
    }

}
