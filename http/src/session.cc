#include "session.h"
#include <iostream>

rwg_http::session::session(int fd,
                           rwg_http::buffer_pool& pool,
                           std::function<void (rwg_http::buf_outstream&)> need_outsync_func,
                           std::function<void ()> close_callback)
    : _fd(fd)
    , _req(std::make_shared<rwg_http::req>(fd, pool.alloc(32), std::bind(&rwg_http::session::close, this)))
    , _res(std::make_shared<rwg_http::res>(fd, pool.alloc(32), need_outsync_func, std::bind(&rwg_http::session::close, this)))
    , _pool(pool)
    , _closed_flag(false)
    , _close_callback(close_callback)
    , _reading(false) {
}

void rwg_http::session::close() {
    if (this->_closed_flag) {
        return;
    }
    this->_closed_flag = true;
    this->_req->close();
    this->_res->close();
    this->_close_callback();
}

rwg_http::req& rwg_http::session::req() {
    return *this->_req;
}

rwg_http::res& rwg_http::session::res() {
    return *this->_res;
}

std::atomic<bool>& rwg_http::session::closed_flag() {
    return this->_closed_flag;
}


