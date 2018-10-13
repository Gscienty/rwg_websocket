#pragma once

#include "buffer_pool.h"
#include "req.h"
#include "res.h"
#include <memory>

namespace rwg_http {

class session {
private:
    int _fd;
    std::unique_ptr<rwg_http::req> _req;
    std::unique_ptr<rwg_http::res> _res;

    rwg_http::buffer_pool& _pool;
public:
    session(int fd, rwg_http::buffer_pool& pool, std::function<void (rwg_http::buf_outstream&)> notify_func);

    rwg_http::req& req();
    rwg_http::res& res();
};

}
