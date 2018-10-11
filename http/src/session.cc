#include "session.h"

rwg_http::session::session(int fd, rwg_http::buffer_pool& pool)
    : _fd(fd)
    , _pool(pool) {

    auto buf = pool.alloc(pool.unit_size() * 4);
    this->_req.reset(new rwg_http::req(fd, buf.split(buf.unit_size()), buf.split(buf.unit_size())));
    this->_res.reset(new rwg_http::res(fd, buf.split(buf.unit_size()), buf.split(buf.unit_size())));

    /* this->_req.reset(new rwg_http::req(fd, pool.alloc(pool.unit_size()), pool.alloc(pool.unit_size()))); */
    /* this->_res.reset(new rwg_http::res(fd, pool.alloc(pool.unit_size()), pool.alloc(pool.unit_size()))); */
}

rwg_http::req& rwg_http::session::req() {
    return *this->_req;
}

rwg_http::res& rwg_http::session::res() {
    return *this->_res;
}

