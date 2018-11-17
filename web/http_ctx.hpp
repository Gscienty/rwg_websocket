#ifndef _RWG_WEB_HTTP_CTX_
#define _RWG_WEB_HTTP_CTX_

#include "web/abstract_in_event.hpp"
#include "web/res.hpp"
#include "web/req.hpp"
#include "websocket/websocket_startup.hpp"
#include <functional>

namespace rwg_web {

class http_ctx {
private:
    int _fd;
    rwg_websocket::startup &_websocket;
    bool _security;

    std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)> _http_handler;
    rwg_web::req _req;
    rwg_web::res _res;
public:
    http_ctx(int fd,
             rwg_websocket::startup &,
             std::function<void (rwg_web::req & req, rwg_web::res &, std::function<void ()>)>);
    virtual ~http_ctx();

    rwg_web::req &req();
    rwg_web::res &res();
    void use_security(SSL *&, bool use = true);
    void run(std::function<void ()>);
};

}

#endif
