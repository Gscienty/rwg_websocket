#ifndef _RWG_WEB_RES_
#define _RWG_WEB_RES_

#include <string>
#include <map>
#include <unistd.h>
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_web {

static const char __crlf[] = { '\r', '\n' };
static const char __param_delimiter[] = { ':', ' ' };

class res {
private:
    int _fd;
    std::string _version;
    short _status_code;
    std::string _description;
    std::map<std::string, std::string> _header_parameters;

    char * _buf;
    int _buf_pos;
    int _buf_size;

    bool __putc(char c) {
        if (this->_buf_pos == this->_buf_size) {
            return this->flush();
        }
        this->_buf[this->_buf_pos++] = c;
        if (this->_buf_pos == this->_buf_size) {
            this->flush();
        }
        return true;
    }
public:
    res()
        : _fd(0)
        , _buf(nullptr)
        , _buf_pos(0)
        , _buf_size(0) {
    }

    virtual ~res() {
        this->free_buf();
    }

    void reset() {
        this->_version.clear();
        this->_status_code = 0;
        this->_description.clear();
        this->_header_parameters.clear();
    }

    int& fd() { return this->_fd; }

    void alloc_buf(int size) {
        if (this->_buf != nullptr) {
            delete [] this->_buf;
        }

        this->_buf = new char[size];
        this->_buf_pos = 0;
        this->_buf_size = size;
    }
    void free_buf() {
        if (this->_buf != nullptr) {
            delete [] this->_buf;
        }

        this->_buf = nullptr;
        this->_buf_pos = 0;
        this->_buf_size = 0;
    }

    std::string& version() { return this->_version; }
    short& status_code() { return this->_status_code; }
    std::string& description() { return this->_description; }
    std::map<std::string, std::string>& header_parameters() { return this->_header_parameters; }

    void write_header() {
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

    template<typename _T_Input_Itr> void write(_T_Input_Itr begin, _T_Input_Itr end) {
        for (_T_Input_Itr itr = begin; itr != end; itr++) {
            this->__putc(*itr);
        }
    }

    bool flush() { 
        if (this->_buf_pos == 0) {
            return false;
        }
        ::write(this->_fd, this->_buf, this->_buf_pos);
        this->_buf_pos = 0;
#ifdef DEBUG
        std::cout << "response flush" << std::endl;
#endif
        return true;
    }
};

}

#endif
