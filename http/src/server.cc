#include "server.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

rwg_http::server::server(int event_size)
    : _epfd(::epoll_create(event_size))
    , _server_fd(::socket(AF_INET, SOCK_STREAM, 0))
    , _backlog(event_size) {
}

void rwg_http::server::init_buffer_pool(std::size_t pool_size) {
    this->_buffer_pool.reset(new rwg_http::buffer_pool(pool_size));
}

void rwg_http::server::init_thread_pool(std::size_t pool_size) {
    this->_thread_pool.reset(new rwg_http::thread_pool(pool_size));
}

void rwg_http::server::listen(const char* ip, std::uint16_t port) {
    this->_addr.sin_family = AF_INET;
    this->_addr.sin_addr.s_addr = ::inet_addr(ip);
    this->_addr.sin_port = ::htons(port);

    ::bind(this->_server_fd, reinterpret_cast<::sockaddr*>(&this->_addr), sizeof(::sockaddr_in));
    ::listen(this->_server_fd, this->_backlog);
}

void rwg_http::server::start(std::size_t batch_size, int timeout) {
    this->_server_event.events = EPOLLIN | EPOLLET;
    this->_server_event.data.fd = this->_server_fd;
    epoll_ctl(this->_epfd, EPOLL_CTL_ADD, this->_server_fd, &this->_server_event);

    ::epoll_event events[batch_size];

    while (true) {
        int avail_count = ::epoll_wait(this->_epfd, events, batch_size, timeout);

        for (auto i = 0; i < avail_count; i++) {
            ::epoll_event& event = events[i];
            if (event.data.fd == this->_server_fd) {
                if (event.events & EPOLLIN) {
                    // new client
                    this->_thread_pool->submit(std::bind(&rwg_http::server::__accept, this));
                }
            }
            else {
                // session
                if (event.events & EPOLLIN) {
                    auto session_ptr = reinterpret_cast<rwg_http::session*>(event.data.ptr);
                    if (!session_ptr->reading()) {
                        session_ptr->reading() = true;
                        this->_thread_pool->submit(std::bind(&rwg_http::server::__recv_data, this, session_ptr));
                    }
                }
                if (event.events & EPOLLRDHUP) {
                    reinterpret_cast<rwg_http::session*>(event.data.ptr)->close();
                    this->__close(event.data.fd);
                }
                if (event.events & EPOLLERR) {
                    reinterpret_cast<rwg_http::session*>(event.data.ptr)->close();
                    this->__close(event.data.fd);
                }
            }
        }
    }
}

void rwg_http::server::__accept() {
    ::sockaddr_in addr;
    ::socklen_t addr_len;
    int cfd = ::accept(this->_server_fd, reinterpret_cast<::sockaddr*>(&addr), &addr_len);

    auto session_ptr =
        std::make_shared<rwg_http::session>(cfd,
                                            *this->_buffer_pool,
                                            std::bind(&rwg_http::server::__send_data, this, std::placeholders::_1),
                                            std::bind(&rwg_http::server::__close, this, cfd));

    {
        std::lock_guard<std::mutex> lck(this->_mtx);
        this->_sessions[cfd] = session_ptr;
    }

    session_ptr->event().events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    session_ptr->event().data.fd = cfd;
    session_ptr->event().data.ptr = reinterpret_cast<void*>(session_ptr.get());

    ::epoll_ctl(this->_epfd, EPOLL_CTL_ADD, cfd, &session_ptr->event());

    this->_chain.execute(*session_ptr);

    session_ptr->close();
}

void rwg_http::server::__recv_data(rwg_http::session* session) {
    session->req().sync();
    session->reading() = false;
}

void rwg_http::server::__send_data(rwg_http::buf_outstream& out) {
    out.nonblock_sync();
}

void rwg_http::server::__close(int cfd) {
    std::lock_guard<std::mutex> lck(this->_mtx);
    auto itr = this->_sessions.find(cfd);
    if (itr != this->_sessions.end()) {
        this->_sessions.erase(itr);
        ::close(cfd);
    }
}

rwg_http::chain_middleware& rwg_http::server::chain() {
    return this->_chain;
}
