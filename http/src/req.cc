#include "req.h"
#include <unistd.h>

rwg_http::req::req(int fd, rwg_http::buffer& buffer, rwg_http::buffer&& cache)
    : _fd(fd)
    , _str(buffer)
    , _cache(cache) {

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

void rwg_http::req::sync() {
    std::size_t size = ::read(this->_fd, this->_cache.unit(0), this->_cache.size());
    this->_str.load(this->_cache.unit(0), this->_cache.unit(0) + size);
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
