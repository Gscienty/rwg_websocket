#include "session.h"

rwg_http::session::session(int fd, rwg_http::buffer_pool& pool, std::function<void (rwg_http::buf_outstream&)> notify_func)
    : _fd(fd)
    , _pool(pool) {

    this->_req.reset(new rwg_http::req(fd, pool.alloc(32)));
    this->_res.reset(new rwg_http::res(fd, pool.alloc(32), notify_func));
}

rwg_http::req& rwg_http::session::req() {
    return *this->_req;
}

rwg_http::res& rwg_http::session::res() {
    return *this->_res;
}

