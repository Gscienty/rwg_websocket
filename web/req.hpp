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
    req_stat_header,
    req_stat_header_end,
    req_stat_raw,
    req_stat_interrupt,
    req_stat_end
};

static const char __crlf_status[] = { '\r', '\n', '\r', '\n' };

class req {
private:
    int _fd;
    req_stat _stat;
    const int _buf_size;
    char * const _buf;
    int _buf_avail_size;
    int _buf_pos;
    char *_raw;

    std::string _method;
    std::string _uri;
    std::string _version;

    std::map<std::string, std::string> _header_parameters;

    void __parse_header() {
        int stat = 0;
        int hp_stat = 0;
        std::stringstream sstr;
        while (this->_stat == rwg_web::req_stat::req_stat_header) {
            if (this->_buf_pos == this->_buf_avail_size) {
                this->load();
                if (this->_buf_pos == this->_buf_avail_size) {
                    this->_stat = rwg_web::req_stat::req_stat_interrupt;
                    return;
                }
            }

            char c = this->_buf[this->_buf_pos++];
            switch (stat) {
            case 0:
                if (c == ' ') { stat++; }
                else { this->_method.push_back(c); }
                break;
            case 1:
                if (c == ' ') { stat++; }
                else { this->_uri.push_back(c); }
                break;
            case 2:
                if (c == '\r') {
                    stat++;
                    hp_stat++;
                }
                else { this->_version.push_back(c); }
                break;
            case 3:
                if (c == __crlf_status[hp_stat]) {
                    if (hp_stat == 0) {
                        std::string line = sstr.str();
                        sstr.str("");
                        std::size_t delimiter = line.find(':');
                        std::string key = line.substr(0, delimiter);
                        std::string value = line.substr(delimiter + 2);

                        this->_header_parameters.insert(std::make_pair(key, value));
                    }
                    hp_stat++;
                    if (hp_stat == 4) {
                        this->_stat = rwg_web::req_stat::req_stat_header_end;
                    }
                }
                else {
                    hp_stat = 0;
                    sstr.put(c);
                }
                break;
            }
        }
    }
public:
    req(int fd, int size)
        : _fd(fd)
        , _stat(req_stat_header) 
        , _buf_size(size)
        , _buf(new char[size])
        , _buf_avail_size(0)
        , _raw(nullptr) {
    }

    virtual ~req() {
        delete [] this->_buf;
    }

    void load() {
#ifdef DEBUG
        std::cout << "req load data" << std::endl;
#endif
        this->_buf_pos = 0;
        this->_buf_avail_size = ::read(this->_fd, this->_buf, this->_buf_size);
    }

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
};

}

#endif
