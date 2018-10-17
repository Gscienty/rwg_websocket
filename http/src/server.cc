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
        std::lock_guard<std::mutex> lck(this->_mtx);

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
                    auto session = this->_sessions[event.data.fd];
                    if (session->closed_flag()) {
                        session->close();
                    }
                    else if (!session->in_chain()) {
                        this->_thread_pool->submit(std::bind(&rwg_http::server::__execute_chain, this, session));
                    }
                    else if (!session->reading()) {
                        session->reading() = true;
                        this->__recv_data(session);
                    }
                }
                if (event.events & EPOLLRDHUP) {
                    auto session = this->_sessions[event.data.fd];
                    session->close();
                }
                if (event.events & EPOLLERR) {
                    auto session = this->_sessions[event.data.fd];
                    session->close();
                }
            }
        }
    }
}

void rwg_http::server::__accept() {
#ifdef DEBUG
    std::cout << "server accept" << std::endl;
#endif
    ::sockaddr_in addr;
    ::socklen_t addr_len;
    int cfd = ::accept(this->_server_fd, reinterpret_cast<::sockaddr*>(&addr), &addr_len);

    auto session =
        std::make_shared<rwg_http::session>(cfd,
                                            *this->_buffer_pool,
                                            std::bind(&rwg_http::server::__send_data, this, std::placeholders::_1),
                                            std::bind(&rwg_http::server::__close, this, cfd));

    {
        std::lock_guard<std::mutex> lck(this->_mtx);
        this->_sessions[cfd] = session;
        session->event().events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
        session->event().data.fd = cfd;

        ::epoll_ctl(this->_epfd, EPOLL_CTL_ADD, cfd, &session->event());
    }

    this->__execute_chain(session);
}

void rwg_http::server::__recv_data(std::shared_ptr<rwg_http::session> session) {
#ifdef DEBUG
    std::cout << "server::__recv_data: begin" << std::endl;
#endif
    if (session->closed_flag()) {
#ifdef DEBUG
        std::cout << "server::__recv_data: closed" << std::endl;
#endif
        return;
    }
    session->req().sync();
    session->reading() = false;
#ifdef DEBUG
    std::cout << "server::__recv_data: end" << std::endl;
#endif
}

void rwg_http::server::__send_data(rwg_http::buf_outstream& out) {
#ifdef DEBUG
    std::cout << "server::__send_data: begin" << std::endl;
#endif
    out.nonblock_sync();
#ifdef DEBUG
    std::cout << "server::_send_data: end" << std::endl;
#endif
}

void rwg_http::server::__close(int cfd) {
#ifdef DEBUG
    std::cout << cfd << " server::__closed: prebegin" << std::endl;
#endif
    std::lock_guard<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << cfd << " server::__closed: begin" << std::endl;
#endif
    auto itr = this->_sessions.find(cfd);
    if (itr != this->_sessions.end()) {
        epoll_ctl(this->_epfd, EPOLL_CTL_DEL, cfd, &itr->second->event());
        this->_sessions.erase(itr);
        ::close(cfd);
#ifdef DEBUG
        std::cout << cfd << " server::__closed: delete" << std::endl;
#endif
    }
#ifdef DEBUG
    std::cout << cfd << " server__closed: end" << std::endl;
#endif
}

rwg_http::chain_middleware& rwg_http::server::chain() {
    return this->_chain;
}

void rwg_http::server::__execute_chain(std::shared_ptr<rwg_http::session> session) {
#ifdef DEBUG
    std::cout << "server::__execute_chain: begin" << std::endl;
#endif
    if (session->closed_flag()) {
#ifdef DEBUG
        std::cout << "server::__execute_chain: closed" << std::endl;
#endif
        return;
    }
    session->in_chain() = true;
    session->req().set_buffer(this->_buffer_pool->alloc(32));
    session->res().set_buffer(this->_buffer_pool->alloc(32));
    
#ifdef DEBUG
    std::cout << "server::__execute_chain: execute begin" << std::endl;
#endif
    this->_chain.execute(*session);
#ifdef DEBUG
    std::cout << "server::__execute_chain: execute end" << std::endl;
#endif

    session->req().release();
    session->res().release();
    session->in_chain() = false;
#ifdef DEBUG
    std::cout << "server::__execute_chain: end" << std::endl;
#endif
}
