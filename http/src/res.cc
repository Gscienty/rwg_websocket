#include "res.h"
#include <unistd.h>

rwg_http::res::res(int fd, rwg_http::buffer&& buffer, rwg_http::buffer&& cache)
    : _fd(fd)
    , _cache(std::move(cache))
    , _str(std::move(buffer))
    , _version("HTTP/1.1")
    , _status_code(200)
    , _description("OK") {

    this->_str.set_readable_event(std::bind(&rwg_http::res::sync, this));
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

void rwg_http::res::flush_header() {
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

void rwg_http::res::sync() {
    std::size_t n = this->_str.read(this->_cache.unit(0), this->_cache.size());
    ::write(this->_fd, this->_cache.unit(0), n);
}

