#ifndef _RWG_WEB_HTTP_CTX_
#define _RWG_WEB_HTTP_CTX_

#include "web/abstract_in_event.hpp"
#include "web/res.hpp"
#include "web/req.hpp"
#include "websocket/websocket_startup.hpp"
#include <functional>
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_web {

class http_ctx {
private:
    rwg_websocket::startup &_websocket;

    std::function<void (rwg_web::req&, rwg_web::res&)> _http_handler;
    rwg_web::req _req;
    rwg_web::res _res;
public:
    http_ctx(rwg_websocket::startup &websocket,
             std::function<void (rwg_web::req& req, rwg_web::res&)> handler)
        : _websocket(websocket)
        , _http_handler(handler) {}

    rwg_web::req& req() { return this->_req; }
    rwg_web::res& res() { return this->_res; }

    void run(int fd, std::function<void ()> close_cb) {
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
            else if (this->_req.stat() == rwg_web::req_stat::req_stat_header_end) {
#ifdef DEBUG
                std::cout << "execute start http_ctx run handle" << std::endl;
#endif

                this->_res.alloc_buf(512);
                if (!this->_websocket.handshake(this->_req, this->_res)) {
#ifdef DEBUG
                    std::cout << "normal http" << std::endl;
#endif
                    this->_http_handler(this->_req, this->_res);
                }
                this->_res.free_buf();
            }
            this->_req.free_buf();
        }
    }

    virtual ~http_ctx() {
    }
};

}

#endif
