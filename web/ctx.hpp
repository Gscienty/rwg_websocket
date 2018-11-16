#ifndef _RWG_WEB_CTX_
#define _RWG_WEB_CTX_

#include "web/abstract_in_event.hpp"
#include "web/http_ctx.hpp"
#include "websocket/websocket_startup.hpp"
#include <memory>
#include <functional>
#include <openssl/ssl.h>

namespace rwg_web {

class ctx : public rwg_web::abstract_in_event {
private:
    bool _security;
    bool _sec_inited;
    SSL *_ssl;
    rwg_web::http_ctx _http_ctx;
    rwg_websocket::startup &_websocket;
    std::function<void (int)>_close_cb;
    bool _close_flag;

    void __close();
public:
    ctx(int fd,
        std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)>,
        rwg_websocket::startup &,
        std::function<void (int)>);
    virtual ~ctx();

    virtual void in_event() override;
    bool use_security(SSL_CTX *);
    void close();

};
}

#endif
