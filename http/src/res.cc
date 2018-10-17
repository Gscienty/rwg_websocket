#include "res.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef DEBUG
#include <iostream>
#endif

rwg_http::res::res(int fd,
                   std::function<void (rwg_http::buf_outstream&)> notify_func,
                   std::function<void ()> close_callback)
    : _fd(fd)
    , _str(16,
           std::bind(&rwg_http::res::__sync,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2),
           notify_func,
           close_callback,
           fd)
    , _version("HTTP/1.1")
    , _status_code(200)
    , _description("OK")
    , _close_flag(false) {
}

rwg_http::res::~res() {
#ifdef DEBUG
    std::cout << "res dtor" << std::endl;
#endif
    this->release();
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
#ifdef DEBUG
    std::cout << "res::write_header: begin" << std::endl;
#endif
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
#ifdef DEBUG
    std::cout << "res::write_header: end" << std::endl;
#endif
}

bool rwg_http::res::__sync(std::uint8_t* s, std::size_t n) {
#ifdef DEBUG
    std::cout << "res::__sync: begin" << std::endl;
#endif
    if (this->_close_flag) {
#ifdef DEBUG
        std::cout << "res::__sync: closed" << std::endl;
#endif
        return false;
    }
    int size = ::write(this->_fd, s, n);
    if (size <= 0) {
#ifdef DEBUG
        std::cout << "res::__sync: size <= 0" << std::endl;
#endif
        this->close();
        return false;
    }
#ifdef DEBUG
    std::cout << "res::__sync: end" << std::endl;
#endif
    return true;
}

void rwg_http::res::close() {
#ifdef DEBUG
    std::cout << "res::close: begin" << std::endl;
#endif
    if (this->_close_flag) {
#ifdef DEBUG
        std::cout << "res::close: closed" << std::endl;
#endif
        return;
    }
    this->_str.close();
    this->_close_flag = true;
#ifdef DEBUG
    std::cout << "res::close: end" << std::endl;
#endif
}

void rwg_http::res::flush() {
#ifdef DEBUG
    std::cout << "res::flush: begin" << std::endl;
#endif
    this->_str.flush();
#ifdef DEBUG
    std::cout << "res::flush: end" << std::endl;
#endif
}

void rwg_http::res::clear() {
    this->_version = "HTTP/1.1";
    this->_status_code = 200;
    this->_description = "OK";
    this->_header_parameters.clear();
}

void rwg_http::res::set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer) {
    this->_str.set_buffer(std::move(buffer));
}

void rwg_http::res::release() {
    this->_str.release();
}
