#include "web/server.hpp"

namespace rwg_web {
server::server(int epsize, int events_size, int ep_wait_timeout) 
    : _procs_count(::sysconf(_SC_NPROCESSORS_CONF))
    , _epsize(epsize)
    , _events_size(events_size)
    , _ep_wait_timeout(ep_wait_timeout)
    , _loop_itr(0) {
    this->_main_epfd = ::epoll_create(this->_epsize);

    for (auto i = 0; i < this->_procs_count; i++) {
        int epfd = ::epoll_create(this->_epsize);
        this->_eps.push_back(epfd);
        this->_threads.push_back(std::thread(std::bind(&rwg_web::server::__thread_main, this, epfd)));
        this->_threads.back().detach();
    }
}

server::~server() {}

void server::__close(int fd) {
#ifdef DEBUG
    std::cout << "close event" << std::endl;
#endif
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
                std::cout << "thread[" << epfd << "]: server received events: [" << events_count << ']' << std::endl;
            }
        }
        else {
            std::cout << ::strerror(errno) << std::endl;
        }
#endif

        for (auto i = 0; i < events_count; i++) {
            auto& event = events[i];

            if (event.events & EPOLLRDHUP) {
#ifdef DEBUG
                std::cout << "fd[" << event.data.fd << "]: remote close" << std::endl;
#endif
                this->__close(event.data.fd);
            }
            else if (event.events & EPOLLIN) {
#ifdef DEBUG
                std::cout << "fd[" << event.data.fd << "]: epoll in" << std::endl;
#endif
                auto action_itr = this->_fd_in_events.find(event.data.fd);
                if (action_itr != this->_fd_in_events.end()) {
#ifdef DEBUG
                    std::cout << "find action" << std::endl;
#endif
                    action_itr->second->in_event();
                }
                else {
#ifdef DEBUG
                    std::cout << "cannot find action" << std::endl;
#endif
                }
            }
        }
    }
}

void server::in_event() {
#ifdef DEBUG
    std::cout << "server[" << this->ep_event().data.fd << "] accept" << std::endl;
#endif
    ::sockaddr_in c_addr;
    ::socklen_t c_addr_len;
    int c_fd = ::accept(this->ep_event().data.fd, reinterpret_cast<::sockaddr *>(&c_addr), &c_addr_len);

    // set nonblock
    int flags = ::fcntl(c_fd, F_GETFL, 0);
    if (flags == -1) {
        flags = 0;
    }
    ::fcntl(c_fd, F_SETFL, flags | O_NONBLOCK);

    int epfd = this->_eps[this->_loop_itr];
    this->_loop_itr = (this->_loop_itr + 1) % this->_eps.size();

#ifdef DEBUG
    std::cout << "remote IP: [" << c_addr.sin_addr.s_addr << "]; port: [" << c_addr.sin_port << "]; client[" << c_fd << "] added to epoll[" << epfd << "]" << std::endl;
#endif

    auto ctx = new rwg_web::ctx(this->_http_handler,
                                this->_websocket,
                                std::bind(&rwg_web::server::__close, this, std::placeholders::_1));
    ctx->ep_event().data.fd = c_fd;

    this->_fd_in_events[c_fd] = ctx;
    ::epoll_ctl(epfd, EPOLL_CTL_ADD, c_fd, &ctx->ep_event());
}

void server::listen(std::string ip, short port) {
    int sfd = ::socket(AF_INET, SOCK_STREAM, 0);
#ifdef DEBUG
    std::cout << "create socket[" << sfd << "]" << std::endl;
#endif

    ::sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = ::inet_addr(ip.c_str());
    s_addr.sin_port = ::htons(port);

    ::bind(sfd, reinterpret_cast<::sockaddr *>(&s_addr), sizeof(::sockaddr_in));
    ::listen(sfd, this->_procs_count * this->_epsize);

    this->ep_event().events = EPOLLIN;
    this->ep_event().data.fd = sfd;

    this->_fd_in_events[sfd] = this;

    ::epoll_ctl(this->_main_epfd, EPOLL_CTL_ADD, sfd, &this->ep_event());
}

void server::http_handle(std::function<void (rwg_web::req &, rwg_web::res &, std::function<void ()>)> handler) {
    this->_http_handler = handler;
}

void server::websocket_frame_handle(std::function<void (rwg_websocket::frame &, std::function<void ()>)> handler) {
    this->_websocket.frame_handle(handler);
}

void server::websocket_handshake_handle(std::function<bool (rwg_web::req &)> handler) {
    this->_websocket.handshake_handle(handler);
}

void server::websocket_remove_handle(std::function<void (int)> handler) {
    this->_websocket.remove_handle(handler);
}

void server::websocket_init_handle(std::function<void (int)> handler) {
    this->_websocket.init_handle(handler);
}

void server::start() {
    this->__thread_main(this->_main_epfd);
}

void server::close_handle(std::function<void (int)> func) {
    this->_close_cb = func;
}
}
