#ifndef _RWG_WEB_REQ_
#define _RWG_WEB_REQ_

#include <unistd.h>
#include <string>
#include <map>
#include <sstream>
#include <openssl/ssl.h>

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


class req {
private:
    int _fd;
    bool _security;
    SSL *_ssl;

    req_stat _stat;
    int _buf_size;
    char * _buf;
    std::uint64_t _buf_avail_size;
    std::uint64_t _buf_pos;
    char *_raw;
    std::uint64_t _raw_len;
    std::uint64_t _raw_readed_len;

    std::string _method;
    std::string _uri;
    std::string _version;

    std::map<std::string, std::string> _header_parameters;

    void __load();
    void __parse_header();
    void __parse_raw();
public:
    req();
    virtual ~req();

    void use_security(SSL *, bool use = true);
    void reset();
    void alloc_buf(int size);
    void free_buf();
    rwg_web::req_stat stat() const;
    void parse();

    int &fd();
    SSL *&ssl();
    std::string &method();
    std::string &uri();
    std::string &version();
    char *raw() const;
    std::uint64_t raw_len() const;
    std::map<std::string, std::string>& header_parameters();
};

}

#endif
