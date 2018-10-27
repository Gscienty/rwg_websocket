#ifndef _RWG_WEB_REQ_
#define _RWG_WEB_REQ_

#include <unistd.h>
#include <string>
#include <map>
#include <sstream>
#ifdef DEBUG 
#include <iostream>
#endif

namespace rwg_web {

enum req_stat {
    req_stat_header_method,
    req_stat_header_uri,
    req_stat_header_verison,
    req_stat_header_req_end,
    req_stat_header_param,
    req_stat_header_param_end1,
    req_stat_header_param_end2,
    req_stat_header_param_end3,
    req_stat_header_end,
    req_stat_raw,
    req_stat_interrupt,
    req_stat_end,
    req_stat_err,
};

static const char __crlf_status[] = { '\r', '\n', '\r', '\n' };

class req {
private:
    int _fd;
    req_stat _stat;
    int _buf_size;
    char * _buf;
    int _buf_avail_size;
    int _buf_pos;
    char *_raw;

    std::string _method;
    std::string _uri;
    std::string _version;

    std::map<std::string, std::string> _header_parameters;

    void __parse_header() {
#ifdef DEBUG
        std::cout << "request parse header" << std::endl;
#endif
        if (this->_stat == rwg_web::req_stat::req_stat_end ||
            this->_stat == rwg_web::req_stat::req_stat_interrupt ||
            this->_stat == rwg_web::req_stat::req_stat_header_end ||
            this->_stat == rwg_web::req_stat::req_stat_err) {
#ifdef DEBUG
            std::cout << "request parsed header end" << std::endl;
#endif
            return;
        }

        std::stringstream sstr;
        while (this->_stat != rwg_web::req_stat::req_stat_end &&
               this->_stat != rwg_web::req_stat::req_stat_interrupt &&
               this->_stat != rwg_web::req_stat::req_stat_header_end &&
               this->_stat != rwg_web::req_stat::req_stat_err) {
            if (this->_buf_pos == this->_buf_avail_size) {
                this->load();
                if (this->_stat == rwg_web::req_stat::req_stat_interrupt ||
                    this->_stat == rwg_web::req_stat::req_stat_err) {
                    break;
                }
            }

            char c;
            switch (this->_stat) {
            default:
                break;
            case rwg_web::req_stat::req_stat_header_method:
                c = this->_buf[this->_buf_pos++];
                if (c == ' ') {
#ifdef DEBUG
                    std::cout << "parse completed req [method]" << std::endl;
#endif
                    this->_stat = rwg_web::req_stat::req_stat_header_uri;
                }
                else { this->_method.push_back(c); }
                break;
            case rwg_web::req_stat::req_stat_header_uri:
                c = this->_buf[this->_buf_pos++];
                if (c == ' ') {
#ifdef DEBUG
                    std::cout << "parse completed req [uri]" << std::endl;
#endif
                    this->_stat = rwg_web::req_stat::req_stat_header_verison;
                }
                else { this->_uri.push_back(c); }
                break;
            case rwg_web::req_stat::req_stat_header_verison:
                c = this->_buf[this->_buf_pos++];
                if (c == '\r') {
#ifdef DEBUG
                    std::cout << "parse completed req [version]" << std::endl;
#endif
                    this->_stat = rwg_web::req_stat_header_req_end;
                }
                else { this->_version.push_back(c); }
                break;
            case rwg_web::req_stat::req_stat_header_req_end:
                c = this->_buf[this->_buf_pos++];
                if (c == '\n') {
#ifdef DEBUG
                    std::cout << "parse completed req req header" << std::endl;
#endif
                    this->_stat = rwg_web::req_stat::req_stat_header_param;
                }
                else {
#ifdef DEBUG
                    std::cout << "parse req req header error" << std::endl;
#endif
                    this->_stat = rwg_web::req_stat::req_stat_err;
                }
                break;
            case rwg_web::req_stat::req_stat_header_param:
                c = this->_buf[this->_buf_pos++];
                if (c == '\r') {
#ifdef DEBUG
                    std::cout << "parse req one parameter" << std::endl;
#endif
                    std::string line = sstr.str();
                    sstr.str("");
                    std::size_t delimiter = line.find(':');
                    std::string key = line.substr(0, delimiter);
                    std::string value = line.substr(delimiter + 2);

                    this->_header_parameters.insert(std::make_pair(key, value));

                    this->_stat = rwg_web::req_stat::req_stat_header_param_end1;
                }
                else {
                    sstr.put(c);
                }
                break;
            case rwg_web::req_stat::req_stat_header_param_end1:
                c = this->_buf[this->_buf_pos++];
                if (c == '\n') {
                    this->_stat = rwg_web::req_stat::req_stat_header_param_end2;
                }
                else {
                    this->_stat = rwg_web::req_stat::req_stat_err;
                }
                break;
            case rwg_web::req_stat::req_stat_header_param_end2:
                c = this->_buf[this->_buf_pos++];
                if (c == '\r') {
                    this->_stat = rwg_web::req_stat::req_stat_header_param_end3;
                }
                else {
#ifdef DEBUG
                    std::cout << "req parse next parameter" << std::endl;
#endif
                    sstr.put(c);
                    this->_stat = rwg_web::req_stat::req_stat_header_param;
                }
                break;
            case rwg_web::req_stat::req_stat_header_param_end3:
                c = this->_buf[this->_buf_pos++];
                if (c == '\n') {
#ifdef DEBUG
                    std::cout << "req completed header" << std::endl;
#endif
                    this->_stat = rwg_web::req_stat::req_stat_header_end;
                }
                else {
                    this->_stat = rwg_web::req_stat::req_stat_err;
                }
                break;
            }
        }
    }
public:
    req()
        : _fd(0)
        , _stat(rwg_web::req_stat::req_stat_header_method)
        , _buf_size(0)
        , _buf(nullptr)
        , _buf_avail_size(0)
        , _raw(nullptr) {
    }

    virtual ~req() {
        this->free_buf();
    }

    int& fd() { return this->_fd; }

    void alloc_buf(int size) {
        if (this->_buf != nullptr) {
            delete [] this->_buf;
        }

        this->_buf_size = size;
        this->_buf = new char[size];
        this->_buf_avail_size = 0;
        this->_buf_pos = 0;
    }

    void free_buf() {
        if (this->_buf != nullptr) {
            delete [] this->_buf;
        }
        this->_buf_size = 0;
        this->_buf = nullptr;
        this->_buf_avail_size = 0;
        this->_buf_pos = 0;
    }

    void load() {
#ifdef DEBUG
        std::cout << "req load data" << std::endl;
#endif
        this->_buf_pos = 0;
        ::ssize_t ret = ::read(this->_fd, this->_buf, this->_buf_size);
        if (ret <= 0) {
            this->_buf_avail_size = 0;
            if (ret == 0) {
                this->_stat = rwg_web::req_stat::req_stat_interrupt;
            }
            else if (ret != EAGAIN) {
                this->_stat = rwg_web::req_stat::req_stat_err;
            }
            return;
        }

        this->_buf_avail_size = ret;
    }

    rwg_web::req_stat stat() const { return this->_stat; }

    void parse() {
        this->__parse_header();
#ifdef DEBUG
        std::cout << "receive http req:" << std::endl;
        std::cout << this->_method << ' ' << this->_uri << ' ' << this->_version << std::endl;

        for (auto kv : this->_header_parameters) {
            std::cout << kv.first << ": " << kv.second << std::endl;
        }
#endif
    }

    std::string& method() { return this->_method; }
    std::string& uri() { return this->_uri; }
    std::string& version() { return this->_version; }

    std::map<std::string, std::string>& header_parameters() { return this->_header_parameters; }
};

}

#endif
