#include "req.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#ifdef DEBUG
#include <iostream>
#endif

rwg_http::req::req(int fd, std::function<void ()> close_callback)
    : _fd(fd)
    , _str(16,
           std::bind(&rwg_http::req::__sync,
                     this,
                     std::placeholders::_1,
                     std::placeholders::_2),
           close_callback,
           fd)
    , _close_flag(false) {
}

rwg_http::req::~req() {
#ifdef DEBUG
    std::cout << this->_fd << " req::dtor" << std::endl;
#endif
    this->release();
}

static const std::uint8_t __crlf[] = { '\r', '\n', '\r', '\n' };

void rwg_http::req::get_general_header() {
#ifdef DEBUG
    std::cout << this->_fd << " req::get_general_header: begin" << std::endl;
#endif
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
#ifdef DEBUG
    std::cout << this->_fd << " req::get_general_header: end" << std::endl;
#endif
}

void rwg_http::req::get_request_header() {
#ifdef DEBUG
    std::cout << this->_fd << " req::get_request_header: begin" << std::endl;
#endif
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
#ifdef DEBUG
    std::cout << this->_fd << " req::get_request_header: end" << std::endl;
#endif
}

void rwg_http::req::get_header() {
#ifdef DEBUG
    std::cout << this->_fd << " req::get_header: begin" << std::endl;
#endif
    this->get_general_header();
    this->get_request_header();
#ifdef DEBUG
    std::cout << this->_fd << " req::get_header: end" << std::endl;
#endif
}

std::size_t rwg_http::req::__sync(std::uint8_t* s, std::size_t n) {
#ifdef DEBUG
    std::cout << this->_fd << " req::__sync: begin" << std::endl;
#endif
    if (this->_close_flag) {
#ifdef DEBUG
        std::cout << this->_fd << " req::__sync: closed" << std::endl;
#endif
        return 0;
    }
    ::ssize_t size = ::read(this->_fd, s, n);
    if (size <= 0) {
#ifdef DEBUG
        std::cout << this->_fd << " req::__sync: size <= 0" << std::endl;
#endif
        this->close();
        return 0;
    }
#ifdef DEBUG
    std::cout << this->_fd << " req::__sync: end" << std::endl;
#endif
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
#ifdef DEBUG
    std::cout << this->_fd << " req::sync: begin" << std::endl;
#endif
    if (this->_close_flag) {
#ifdef DEBUG
        std::cout << this->_fd << " req::sync: closed" << std::endl;
#endif
        return;
    }
    this->_str.sync();
#ifdef DEBUG
    std::cout << this->_fd << " req::sync: end" << std::endl;
#endif
}

void rwg_http::req::close() {
#ifdef DEBUG
    std::cout << this->_fd << " req::close: begin" << std::endl;
#endif
    if (this->_close_flag) {
#ifdef DEBUG
        std::cout << this->_fd << " req::close: closed" << std::endl;
#endif
        return;
    }
    this->_close_flag = true;
    this->_str.close();
#ifdef DEBUG
    std::cout << this->_fd << " req::close: end" << std::endl;
#endif
}

void rwg_http::req::clear() {
#ifdef DEBUG
    std::cout << this->_fd << " req::clear begin" << std::endl;
#endif
    this->_version.clear();
    this->_method.clear();
    this->_uri.clear();
    this->_header_parameters.clear();
#ifdef DEBUG
    std::cout << this->_fd << " req::clear end" << std::endl;
#endif
} 

void rwg_http::req::set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer) {
    this->_str.set_buffer(std::move(buffer));
}

void rwg_http::req::release() {
#ifdef DEBUG
    std::cout << this->_fd << " req::release begin" << std::endl;
#endif
    this->_str.release();
#ifdef DEBUG
    std::cout << this->_fd << " req::release end" << std::endl;
#endif
}
