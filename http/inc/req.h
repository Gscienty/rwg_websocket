#pragma once

#include "buf_stream.h"
#include "buffer_pool.h"
#include <string>
#include <map>
#include <functional>
#include <atomic>

namespace rwg_http {

class req {
private:
    int _fd;
    rwg_http::buf_instream _str;

    std::string _version;
    std::string _method;
    std::string _uri;
    std::map<std::string, std::string> _header_parameters;

    void get_general_header();
    void get_request_header();
    std::size_t __sync(std::uint8_t* s, std::size_t n);

    std::atomic<bool> _close_flag;
public:
    req(int fd, std::function<void ()> close_callback);
    virtual ~req();

    std::string& version();
    std::string& method();
    std::string& uri();
    std::map<std::string, std::string>& header_parameters();

    void get_header();

    void sync();
    void close();
    void clear();
    void set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer);
    void release();

    std::atomic<bool>& close_flag() { return this->_close_flag; }
};

}
