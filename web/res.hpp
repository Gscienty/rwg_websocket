#ifndef _RWG_WEB_RES_
#define _RWG_WEB_RES_

#include <string>
#include <map>
#include <unistd.h>
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_web {

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

    bool __putc(char);
public:
    res();
    virtual ~res();

    void reset();
    int& fd();
    void alloc_buf(int);
    
    void free_buf();
    std::string& version();
    short& status_code();
    std::string& description();
    std::map<std::string, std::string>& header_parameters();
    void write_header();
    bool flush();
    template<typename _T_Input_Itr> void write(_T_Input_Itr begin, _T_Input_Itr end) {
        for (_T_Input_Itr itr = begin; itr != end; itr++) {
            this->__putc(*itr);
        }
    }
};
}

#endif
