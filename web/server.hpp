#ifndef _RWG_WEB_SERVER_
#define _RWG_WEB_SERVER_

#include "web/abstract_in_event.hpp"
#include "web/session.hpp"
#include "web/req.hpp"
#include "web/res.hpp"
#include <thread>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <errno.h>
#include <string.h>
#include <map>

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

    rwg_web::session _session;
    bool _shutdown;

    void __thread_main(const int epfd) {
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
                    close(event.data.fd);
                }
                else if (event.events & EPOLLIN) {
#ifdef DEBUG
                    std::cout << "fd[" << event.data.fd << "]: epoll in" << std::endl;
#endif
                    auto action_itr = this->_fd_in_events.find(event.data.fd);
                    if (action_itr != this->_fd_in_events.end()) {
                        action_itr->second->in_event(event.data.fd);
                    }
                }

            }
        }
    }

public:
    server(int epsize, int events_size, int ep_wait_timeout) 
        : _procs_count(::sysconf(_SC_NPROCESSORS_CONF))
        , _epsize(epsize)
        , _events_size(events_size)
        , _ep_wait_timeout(ep_wait_timeout)
        , _loop_itr(0)
        , _shutdown(false) {
        
        this->_main_epfd = ::epoll_create(this->_epsize);

        for (auto i = 0; i < this->_procs_count; i++) {
            int epfd = ::epoll_create(this->_epsize);
            this->_eps.push_back(epfd);
            this->_threads.push_back(std::thread(std::bind(&rwg_web::server::__thread_main, this, epfd)));
            this->_threads.back().detach();
        }
    }

    virtual void in_event(int fd) override {
#ifdef DEBUG
        std::cout << "server[" << fd << "] accept" << std::endl;
#endif
        ::sockaddr_in c_addr;
        ::socklen_t c_addr_len;
        int c_fd = ::accept(fd, reinterpret_cast<::sockaddr *>(&c_addr), &c_addr_len);
        int epfd = this->_eps[this->_loop_itr];
        this->_loop_itr = (this->_loop_itr + 1) % this->_eps.size();

#ifdef DEBUG
        std::cout << "remote IP: [" << c_addr.sin_addr.s_addr << "]; port: [" << c_addr.sin_port << "]; client[" << c_fd << "] added to epoll[" << epfd << "]" << std::endl;
#endif

        ::epoll_event event;
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        event.data.ptr = NULL;
        event.data.fd = c_fd;

        this->_fd_in_events[c_fd] = &this->_session;

        ::epoll_ctl(epfd, EPOLL_CTL_ADD, c_fd, &event);
    }

    void listen(std::string ip, short port) {
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

        this->ep_event().events = EPOLLIN | EPOLLET;
        this->ep_event().data.ptr = this;
        this->ep_event().data.fd = sfd;

        this->_fd_in_events[sfd] = this;

        ::epoll_ctl(this->_main_epfd, EPOLL_CTL_ADD, sfd, &this->ep_event());
    }

    void run(std::function<void (int fd, rwg_web::req&, rwg_web::res&)> executor) {
        this->_session.run(executor);
    }

    void start() {
        this->__thread_main(this->_main_epfd);
    }
};

}

#endif
