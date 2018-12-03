#include "web/server.hpp"
#include "util/debug.hpp"
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#ifdef DEBUG
#include <errno.h>
#endif

namespace rwg_web {

static const long __ssl_ctx_flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_SINGLE_DH_USE | SSL_OP_SINGLE_ECDH_USE;
static const char *__ssl_ctx_cipher_suite = "ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256:DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-SHA384:ECDHE-RSA-AES128-SHA256:ECDHE-RSA-AES256-SHA:ECDHE-RSA-AES128-SHA:DHE-RSA-AES256-SHA256:DHE-RSA-AES128-SHA256:DHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA:ECDHE-RSA-DES-CBC3-SHA:EDH-RSA-DES-CBC3-SHA:AES256-GCM-SHA384:AES128-GCM-SHA256:AES256-SHA256:AES128-SHA256:AES256-SHA:AES128-SHA:DES-CBC3-SHA:HIGH:!aNULL:!eNULL:!EXPORT:!CAMELLIA:!DES:!MD5:!PSK:!RC4";

server::server(int epsize, int events_size, int ep_wait_timeout) 
    : _procs_count(::sysconf(_SC_NPROCESSORS_CONF))
    , _epsize(epsize)
    , _events_size(events_size)
    , _ep_wait_timeout(ep_wait_timeout)
    , _loop_itr(0)
    , _security(false)
    , _cert(nullptr)
    , _pkey(nullptr) {
    this->_main_epfd = ::epoll_create(this->_epsize);

    for (auto i = 0; i < this->_procs_count; i++) {
        int epfd = ::epoll_create(this->_epsize);
        this->_eps.push_back(epfd);
        this->_threads.push_back(std::thread(std::bind(&rwg_web::server::__thread_main, this, epfd)));
        this->_threads.back().detach();
    }
}

server::~server() {
}

void server::__close(int fd) {
    info("close event");
    if (bool(this->_close_cb)) {
        this->_close_cb(fd);
    }
    close(fd);
    this->_websocket.remove(fd);
    auto itr = this->_fd_in_events.find(fd);
    if (itr != this->_fd_in_events.end()) {
        delete itr->second;
        this->_fd_in_events.erase(itr);
    }
}

void server::__thread_main(const int epfd) {
    ::epoll_event events[this->_events_size];
    while (true) {
        auto events_count = ::epoll_wait(epfd, events, this->_events_size, this->_ep_wait_timeout);
#ifdef DEBUG
        if (events_count != -1) {
            if (events_count > 0) {
                info("thread[%d]: server received events: %d", epfd, events_count);
            }
        }
        else {
            error("%s", ::strerror(errno));
        }
#endif

        for (auto i = 0; i < events_count; i++) {
            auto& event = events[i];

            if (event.events & EPOLLRDHUP) {
                info("fd[%d]: remote close", event.data.fd);
                this->__close(event.data.fd);
            }
            else if (event.events & EPOLLIN) {
                info("fd[%d]: epoll in", event.data.fd);
                auto action_itr = this->_fd_in_events.find(event.data.fd);
                if (action_itr != this->_fd_in_events.end()) {
                    info("fd[%d]: finded action", event.data.fd);
                    action_itr->second->in_event();
                }
                else {
                    warn("fd[%d]: cannot find action", event.data.fd);
                }
            }
        }
    }
}

void server::in_event() {
    info("server[%d]: accept", this->ep_event().data.fd);
    ::sockaddr_in c_addr;
    ::socklen_t c_addr_len;
    int c_fd = ::accept(this->ep_event().data.fd, reinterpret_cast<::sockaddr *>(&c_addr), &c_addr_len);

    // set nonblock
    info("client[%d]: set nonblock", c_fd);
    int c_flags = ::fcntl(c_fd, F_GETFL, 0);
    if (c_flags == -1) {
        c_flags = 0;
    }
    if (::fcntl(c_fd, F_SETFL, c_flags | O_NONBLOCK) < 0) {
        warn("client[%d]: set nonblock error", c_fd);
    }

    int epfd = this->_eps[this->_loop_itr];
    this->_loop_itr = (this->_loop_itr + 1) % this->_eps.size();
    info("client[%d]: is added to epoll[%d]", c_fd, epfd);

    auto ctx = new rwg_web::ctx(c_fd,
                                this->_http_handler,
                                this->_websocket,
                                std::bind(&rwg_web::server::__close, this, std::placeholders::_1));

    if (this->_security) {
        if (!ctx->use_security(this->_ssl_ctx)) {
            warn("client[%d]: tls handshake failed", c_fd);
            delete ctx;
            close(c_fd);
            return;
        }
    }

    this->_fd_in_events[c_fd] = ctx;
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, c_fd, &ctx->ep_event());
}

void server::use_security(bool use) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    this->_security = use;

    this->_websocket.use_security(use);
}

void server::init_ssl(const char *cert, const char *key) {
    this->_cert = const_cast<char *>(cert);
    this->_pkey = const_cast<char *>(key);

    this->_ssl_ctx = SSL_CTX_new(TLSv1_2_server_method());
    if (!this->_ssl_ctx) {
        error("SSL_CTX_new: failed");
    }
    SSL_CTX_set_options(this->_ssl_ctx,
                        SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 |
                        SSL_OP_NO_COMPRESSION |
                        SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION);

    SSL_CTX_set_ecdh_auto(this->_ssl_ctx, 1);

    if (SSL_CTX_use_certificate_file(this->_ssl_ctx, cert, SSL_FILETYPE_PEM) != 1) {
        error("SSL_CTX_use_sertificate_file: failed");
    }

    if (SSL_CTX_use_PrivateKey_file(this->_ssl_ctx, key, SSL_FILETYPE_PEM) != 1) {
        error("SSL_CTX_use_PrivateKey_file: failed");
    }

    if (SSL_CTX_check_private_key(this->_ssl_ctx) != 1) {
        error("SSL_CTX_check_private_key: failed");
    }
}

void server::listen(std::string ip, short port) {
    // TODO add reuse && bind/listen check result
    int s_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    info("created socket[%d]", s_fd);

    ::sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
    s_addr.sin_port = ::htons(port);

    ::bind(s_fd, reinterpret_cast<::sockaddr *>(&s_addr), sizeof(::sockaddr_in));
    ::listen(s_fd, this->_procs_count * this->_epsize);

    this->ep_event().events = EPOLLIN;
    this->ep_event().data.fd = s_fd;

    this->_fd_in_events[s_fd] = this;

    ::epoll_ctl(this->_main_epfd, EPOLL_CTL_ADD, s_fd, &this->ep_event());
}

void server::http_handle(std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)> handler) {
    this->_http_handler = handler;
}

void server::websocket_frame_handle(std::function<void (rwg_websocket::endpoint &, std::function<void ()>)> handler) {
    this->_websocket.frame_handle(handler);
}

void server::websocket_handshake_handle(std::function<bool (rwg_web::req &)> handler) {
    this->_websocket.handshake_handle(handler);
}

void server::websocket_remove_handle(std::function<void (rwg_websocket::endpoint &)> handler) {
    this->_websocket.remove_handle(handler);
}

void server::websocket_init_handle(std::function<void (rwg_websocket::endpoint &)> handler) {
    this->_websocket.init_handle(handler);
}

void server::websocket_endpoint_factory(std::function<std::unique_ptr<rwg_websocket::endpoint> (rwg_web::req &)> factory) {
    this->_websocket.endpoint_factory(factory);
}

void server::start() {
    this->__thread_main(this->_main_epfd);
}

void server::close_handle(std::function<void (int)> func) {
    this->_close_cb = func;
}

}
