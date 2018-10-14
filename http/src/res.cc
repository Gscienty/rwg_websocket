#include "res.h"
#include <unistd.h>
#include <iostream>
#include <errno.h>
#include <string.h>

rwg_http::res::res(int fd,
                   rwg_http::buffer&& buffer,
                   std::function<void (rwg_http::buf_outstream&)> notify_func,
                   std::function<void ()> close_callback)
    : _fd(fd)
    , _str(std::move(buffer),
           16,
           std::bind(&rwg_http::res::__sync,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2),
           notify_func,
           close_callback)
    , _version("HTTP/1.1")
    , _status_code(200)
    , _description("OK")
    , _close_flag(false) {
}

std::string& rwg_http::res::version() {
    return this->_version;
}

std::uint16_t& rwg_http::res::status_code() {
    return this->_status_code;
}

std::string& rwg_http::res::description() {
    return this->_description;
}

std::map<std::string, std::string>& rwg_http::res::header_parameters() {
    return this->_header_parameters;
}

static const std::string __crlf("\r\n");
static const std::string __header_param_delimiter(": ");

void rwg_http::res::write_header() {
    // lazy method
    this->_str.write(this->_version.begin(), this->_version.size());
    this->_str.putc(' ');
    std::string status_code = std::to_string(this->_status_code);
    this->_str.write(status_code.begin(), status_code.size());
    this->_str.putc(' ');
    this->_str.write(this->_description.begin(), this->_description.size());
    this->_str.write(__crlf.begin(), __crlf.size());

    for (auto kv : this->_header_parameters) {
        this->_str.write(kv.first.begin(), kv.first.size());
        this->_str.write(__header_param_delimiter.begin(), __header_param_delimiter.size());
        this->_str.write(kv.second.begin(), kv.second.size());
        this->_str.write(__crlf.begin(), __crlf.size());
    }
    this->_str.write(__crlf.begin(), __crlf.size());
}

bool rwg_http::res::__sync(std::uint8_t* s, std::size_t n) {
    if (this->_close_flag) {
        return false;
    }
    int size = ::write(this->_fd, s, n);
    if (size <= 0) {
        this->close();
        return false;
    }
    return true;
}

void rwg_http::res::close() {
    if (this->_close_flag) {
        return;
    }
    this->_str.close();
    this->_close_flag = true;
}

void rwg_http::res::flush() {
    this->_str.flush();
}

void rwg_http::res::clear() {
    this->_version = "HTTP/1.1";
    this->_status_code = 200;
    this->_description = "OK";
    this->_header_parameters.clear();
}

