#include "session.h"
#include "thread_pool.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <iostream>

int main() {
    rwg_http::thread_pool thread_pool(3);
    rwg_http::buffer_pool pool(128);
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

    std::cout << "START\n";

    int cfd = ::accept(sfd, reinterpret_cast<sockaddr*>(&caddr), &caddr_len);

    auto sync_func = [&] (rwg_http::buf_outstream& out) -> void {
        thread_pool.submit([&] () -> void { out.nonblock_sync(); });
    };

    rwg_http::session sess(cfd, pool, sync_func);

    std::cout << "ACCEPTED" << ' ' << cfd << std::endl;

    bool shutdown = false;
    auto thr_func = [&] () -> void {
        ::epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = cfd;
        event.data.ptr = reinterpret_cast<void*>(&sess);

        ::epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &event);
        while (shutdown == false) {
            ::epoll_event eve;
            ::epoll_wait(epfd, &eve, 10, -1);

            reinterpret_cast<rwg_http::session*>(eve.data.ptr)->req().sync();
        }
    };

    std::thread thr(thr_func);
    thr.detach();

    sess.req().get_header();

    std::cout << sess.req().method() << ' ' << sess.req().uri() << ' ' << sess.req().version() << std::endl;

    for (auto kv : sess.req().header_parameters()) {
        std::cout << kv.first << ' ' << kv.second << std::endl;
    }

    sess.res().header_parameters().insert(std::make_pair("Content-Type", "text/plain"));

    sess.res().write_header();
    sess.res().end();

    ::close(cfd);
    ::close(sfd);
    ::close(epfd);

    thread_pool.shutdown();

    return 0;
}
