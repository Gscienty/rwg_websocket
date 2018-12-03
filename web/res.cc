#include "web/res.hpp"
#include "util/debug.hpp"

namespace rwg_web {

static const char __crlf[] = { '\r', '\n' };
static const char __param_delimiter[] = { ':', ' ' };


bool res::__putc(char c) {
    this->_buf.put(c);
    /* if (this->_buf_pos == this->_buf_size) { */
    /*     return this->flush(); */
    /* } */
    /* this->_buf[this->_buf_pos++] = c; */
    /* if (this->_buf_pos == this->_buf_size) { */
    /*     return this->flush(); */
    /* } */
    return true;
}


res::res() : _fd(0) {}

res::~res() {}

void res::use_security(SSL *ssl, bool use) {
    this->_security = use;
    this->_ssl = ssl;
}

void res::reset() {
    this->_version.clear();
    this->_status_code = 0;
    this->_description.clear();
    this->_header_parameters.clear();
}

int &res::fd() {
    return this->_fd;
}

/* void res::alloc_buf(int size) { */
/*     if (this->_buf != nullptr) { */
/*         delete [] this->_buf; */
/*     } */

/*     this->_buf = new char[size]; */
/*     this->_buf_pos = 0; */
/*     this->_buf_size = size; */
/* } */

/* void res::free_buf() { */
/*     if (this->_buf != nullptr) { */
/*         delete [] this->_buf; */
/*     } */

/*     this->_buf = nullptr; */
/*     this->_buf_pos = 0; */
/*     this->_buf_size = 0; */
/* } */

std::string &res::version() {
    return this->_version;
}

short &res::status_code() {
    return this->_status_code;
}

std::string &res::description() {
    return this->_description;
}

std::map<std::string, std::string> &res::header_parameters() {
    return this->_header_parameters;
}

void res::write_header() {
    for (auto c : this->_version) { this->__putc(c); }
    this->__putc(' ');
    std::string status_code = std::to_string(this->_status_code);
    for (auto c : status_code) { this->__putc(c); }
    this->__putc(' ');
    for (auto c : this->_description) { this->__putc(c); }
    for (auto c : __crlf) { this->__putc(c); }

    for (auto kv : this->_header_parameters) {
        for (auto c : kv.first) { this->__putc(c); }
        for (auto c : __param_delimiter) { this->__putc(c); }
        for (auto c : kv.second) { this->__putc(c); }
        for (auto c : __crlf) { this->__putc(c); }
    }

    for (auto c : __crlf) { this->__putc(c); }
}

bool res::flush() { 
    /* if (this->_buf_pos == 0) { */
    /*     return false; */
    /* } */
    if (this->_security) {
        /* SSL_write(this->_ssl, this->_buf, this->_buf_pos); */
        SSL_write(this->_ssl, this->_buf.str().data(), this->_buf.str().size());
    }
    else {
        /* ::write(this->_fd, this->_buf, this->_buf_pos); */
        ::write(this->_fd, this->_buf.str().data(), this->_buf.str().size());
    }
    this->_buf.str(reinterpret_cast<const uint8_t *>(""));
    info("res[%d]: flush", this->_fd);
    return true;
}

}
