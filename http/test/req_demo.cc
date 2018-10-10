#include "req.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>

int main() {
    rwg_http::buffer_pool pool(128, 8);
    int epfd = ::epoll_create(1);

    int sfd = ::socket(AF_INET, SOCK_STREAM, 0);
    ::sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = ::htons(8088);
    saddr.sin_addr.s_addr = ::inet_addr("127.0.0.1");

    ::bind(sfd, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));
    ::listen(sfd, 1);

    ::sockaddr_in caddr;
    ::socklen_t caddr_len;

    int cfd = ::accept(sfd, reinterpret_cast<sockaddr*>(&caddr), &caddr_len);

    rwg_http::buffer cbuffer = pool.alloc(pool.unit_size());
    rwg_http::buffer cache = pool.alloc(pool.unit_size());

    rwg_http::req req(cfd, cbuffer, std::move(cache));

    auto thr_func = [&] () -> void {
        ::epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = cfd;
        event.data.ptr = reinterpret_cast<void*>(&req);

        ::epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &event);
        while (true) {
            ::epoll_event eve;
            ::epoll_wait(epfd, &eve, 10, -1);

            reinterpret_cast<rwg_http::req*>(eve.data.ptr)->sync();
        }
    };

    std::thread thr(thr_func);
    thr.detach();

    req.get_header();

    std::cout << req.method() << ' ' << req.uri() << ' ' << req.version() << std::endl;

    for (auto kv : req.header_parameters()) {
        std::cout << kv.first << ' ' << kv.second << std::endl;
    }


    ::close(cfd);
    ::close(sfd);
    ::close(epfd);

    return 0;
}
