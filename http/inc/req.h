#pragma once

#include "buf_stream.h"
#include "buffer_pool.h"
#include <string>
#include <map>

namespace rwg_http {

class req {
private:
    int _fd;
    rwg_http::buf_stream _str;
    rwg_http::buffer _cache;

    std::string _version;
    std::string _method;
    std::string _uri;
    std::map<std::string, std::string> _header_parameters;

    void get_general_header();
    void get_request_header();
public:
    req(int fd, rwg_http::buffer& buffer, rwg_http::buffer&& cache);

    std::string& version();
    std::string& method();
    std::string& uri();
    std::map<std::string, std::string>& header_parameters();

    void get_header();
    void sync();
};

}
