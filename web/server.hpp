#ifndef _RWG_WEB_SERVER_
#define _RWG_WEB_SERVER_

#include "web/abstract_in_event.hpp"
#include "web/http_ctx.hpp"
#include "web/req.hpp"
#include "web/res.hpp"
#include "web/ctx.hpp"
#include "websocket/websocket_startup.hpp"
#include <thread>
#include <vector>
#include <functional>
#include <map>
#include <openssl/ssl.h>

namespace rwg_web {

class server : public rwg_web::abstract_in_event {
private:
    const int _procs_count;
    const int _epsize;
    const int _events_size;
    const int _ep_wait_timeout;
    int _loop_itr;
    std::vector<std::thread> _threads;
    std::vector<int> _eps;
    int _main_epfd;
    std::map<int, rwg_web::abstract_in_event *> _fd_in_events;
    std::map<int, ::epoll_event> _events;
    std::function<void (int)> _close_cb;
    std::function<void (rwg_web::req&, rwg_web::res&, std::function<void ()>)> _http_handler;
    rwg_websocket::startup _websocket;
    bool _security;
    char *_cert;
    char *_pkey;
    SSL_CTX *_ssl_ctx;

    void __close(int);
    void __thread_main(const int);
public:
    server(int epsize = 1024, int events_size = 16, int ep_wait_timeout = -1);
    virtual ~server();

    virtual void in_event() override;
    void use_security(bool use = true);
    void init_ssl(const char *cert, const char *key);
    void listen(std::string, short);
    void http_handle(std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)>);
    void websocket_frame_handle(std::function<void (rwg_websocket::frame &, std::function<void ()>)>);
    void websocket_handshake_handle(std::function<bool (rwg_web::req &)>);
    void websocket_remove_handle(std::function<void (int)>);
    void websocket_init_handle(std::function<void (int)>);
    void start();
    void close_handle(std::function<void (int)>);
};

}

#endif
