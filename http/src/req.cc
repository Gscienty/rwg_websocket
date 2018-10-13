#include "req.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>

rwg_http::req::req(int fd, rwg_http::buffer&& buffer)
    : _fd(fd)
    , _str(std::move(buffer),
           16,
           std::bind(&rwg_http::req::__sync,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2)) {
}

static const std::uint8_t __crlf[] = { '\r', '\n', '\r', '\n' };

void rwg_http::req::get_general_header() {
    int nth_item = 0;
    int alpha_status = 0;
    while (alpha_status != 2) {
        std::uint8_t c = this->_str.getc();

        if (c == __crlf[alpha_status]) {
            alpha_status++;
        }
        else {
            alpha_status = 0;
            if (c == ' ') {
                nth_item++;
            }
            else {
                switch (nth_item) {
                case 0:
                    this->_method.push_back(c);
                    break;
                case 1:
                    this->_uri.push_back(c);
                    break;
                case 2:
                    this->_version.push_back(c);
                }
            }
        }
    }
}

void rwg_http::req::get_request_header() {
    std::string key_tmp;
    std::string value_tmp;
    bool is_key = true;
    bool start_val = false;

    int alpha_status = 0;
    while (alpha_status != 4) {
        std::uint8_t c = this->_str.getc();
        if (c == __crlf[alpha_status]) {
            alpha_status++;
            if (alpha_status == 1) {
                this->_header_parameters.insert(std::make_pair(std::string(key_tmp), std::string(value_tmp)));

                key_tmp.clear();
                value_tmp.clear();
                is_key = true;
                start_val = false;
            }
        }
        else {
            alpha_status = 0;
            if (is_key && c == ':') {
                is_key = false;
                continue;
            }

            if (is_key) {
                key_tmp.push_back(c);
            }
            else {
                if (start_val == false && c == ' ') { continue; }
                start_val = true;
                value_tmp.push_back(c);
            }
        }
    }
}

void rwg_http::req::get_header() {
    this->get_general_header();
    this->get_request_header();
}

std::size_t rwg_http::req::__sync(std::uint8_t* s, std::size_t n) {
    ::ssize_t size = ::read(this->_fd, s, n);
    if (size == -1) {
        return 0;
    }
    return size;
}

std::string& rwg_http::req::version() {
    return this->_version;
}

std::string& rwg_http::req::method() {
    return this->_method;
}

std::string& rwg_http::req::uri() {
    return this->_uri;
}

std::map<std::string, std::string>& rwg_http::req::header_parameters() {
    return this->_header_parameters;
}

void rwg_http::req::sync() {
    this->_str.sync();
}
