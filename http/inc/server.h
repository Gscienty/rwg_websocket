#pragma once

#include "buffer_pool.h"
#include "thread_pool.h"
#include "session.h"
#include "chain_middleware.h"
#include <memory>
#include <string>
#include <cstdint>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <map>
#include <mutex>

namespace rwg_http {

class server {
private:
    int _epfd;
    int _server_fd;
    int _backlog;
    ::sockaddr_in _addr;
    std::unique_ptr<rwg_http::buffer_pool> _buffer_pool;
    std::unique_ptr<rwg_http::thread_pool> _thread_pool;

    std::map<int, std::shared_ptr<rwg_http::session>> _sessions;
    std::mutex _mtx;

    ::epoll_event _server_event;

    rwg_http::chain_middleware _chain;

    void __accept();
    void __recv_data(rwg_http::session* sess);
    void __send_data(rwg_http::buf_outstream& out);
    void __close(int cfd);
public:
    server(int event_size);
    void init_buffer_pool(std::size_t pool_size);
    void init_thread_pool(std::size_t pool_size);
    void listen(const char* ip, std::uint16_t port);
    void start(std::size_t batch_size, int timeout);

    rwg_http::chain_middleware& chain();
};

}
