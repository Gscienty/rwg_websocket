#pragma once

#include "buf_stream.h"
#include "buffer_pool.h"
#include <string>
#include <map>
#include <cstdint>
#include <mutex>
#include <condition_variable>

namespace rwg_http {

class res {
private:
    int _fd;
    rwg_http::buffer _cache;
    rwg_http::buf_stream _str;

    std::string _version;
    std::uint16_t _status_code;
    std::string _description;

    std::map<std::string, std::string> _header_parameters;

    std::mutex _sync_mtx;
    std::condition_variable _syncable_cond;

    void sync();
public:
    res(int fd, rwg_http::buffer&& buffer, rwg_http::buffer&& cache);

    std::string& version();
    std::uint16_t& status_code();
    std::string& description();
    std::map<std::string, std::string>& header_parameters();

    void flush_header();
};

};
