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

    void __close();
public:
    ctx(std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)>,
        rwg_websocket::startup &,
        std::function<void (int)>);
    virtual ~ctx();

    virtual void in_event() override;
    void close();

};
}

#endif
