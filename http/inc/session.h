#pragma once

#include "buffer_pool.h"
#include "req.h"
#include "res.h"
#include <memory>
#include <atomic>
#include <sys/epoll.h>
#include <atomic>

namespace rwg_http {

class session {
private:
    int _fd;
    std::shared_ptr<rwg_http::req> _req;
    std::shared_ptr<rwg_http::res> _res;

    rwg_http::buffer_pool& _pool;

    std::function<void ()> _req_close;
    std::atomic<bool> _closed_flag;

    std::function<void ()> _close_callback;

    ::epoll_event _event;
    std::atomic<bool> _reading;
    std::atomic<bool> _in_chain;
public:
    session(int fd,
            rwg_http::buffer_pool& pool,
            std::function<void (rwg_http::buf_outstream&)> need_outsync_func,
            std::function<void ()> close_callback);
    virtual ~session();

    rwg_http::req& req();
    rwg_http::res& res();

    std::atomic<bool>& closed_flag();
    void close();

    ::epoll_event& event() { return this->_event; }
    std::atomic<bool>& reading() { return this->_reading; }
    std::atomic<bool>& in_chain() { return this->_in_chain; }
};

}
