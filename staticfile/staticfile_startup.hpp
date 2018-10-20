#ifndef _RWG_STATICFILE_STARTUP_
#define _RWG_STATICFILE_STARTUP_

#include "staticfile/mime.hpp"
#include "web/req.hpp"
#include "web/res.hpp"
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

namespace rwg_staticfile {

class startup {
private:
    std::string _start;
    std::string _path;

    rwg_staticfile::mime _mime;

    void __notfound(rwg_web::req&, rwg_web::res& res) {
        res.status_code() = 404;
        res.description() = "Not Found";
        res.version() = "HTTP/1.1";

        res.header_parameters()["Content-Length"] = "0";
        res.header_parameters()["Connection"] = "keep-alive";

        res.write_header();
    }

    std::size_t __content_length(int fd) {
        ::lseek(fd, 0, SEEK_END);
        std::size_t size = ::lseek(fd, 0, SEEK_CUR);
        ::lseek(fd, 0, SEEK_SET);

        return size;
    }

    std::string __content_type(std::string uri) {
        std::size_t ext_dot = uri.rfind('.');
        if (ext_dot == std::string::npos) {
            return this->_mime.default_mime();
        }
        else {
            std::string ext = uri.substr(ext_dot + 1);
            return this->_mime.get(ext);
        }
    }

    std::string __realpath(std::string uri) {
        return this->_path + uri.substr(this->_start.size());
    }

public:
    startup(std::string start, std::string path)
        : _start(start)
        , _path(path) {}

    bool run(rwg_web::req& req, rwg_web::res& res) {
        if (req.uri().find(this->_start) != 0) {
            return false;
        }

        std::string realpath = this->__realpath(req.uri());
        if (::access(realpath.c_str(), F_OK) != 0 || ::access(realpath.c_str(), R_OK) != 0) {
            this->__notfound(req, res);
            return true;
        }

        int fd = ::open(realpath.c_str(), O_RDONLY);
        std::size_t content_length = this->__content_length(fd);

        res.description() = "OK";
        res.status_code() = 200;
        res.version() = "HTTP/1.1";
        res.header_parameters()["Content-Type"] = this->__content_type(req.uri());
        res.header_parameters()["Connection"] = "keep-alive";
        res.header_parameters()["Content-Length"] = std::to_string(content_length);
        res.write_header();

        std::size_t page_size = ((content_length / _SC_PAGE_SIZE) + (content_length % _SC_PAGE_SIZE == 0 ? 0 : 1)) * _SC_PAGE_SIZE;
        void *mmaped = ::mmap(NULL, page_size, PROT_READ, MAP_SHARED, fd, 0);

        res.write(reinterpret_cast<char *>(mmaped), reinterpret_cast<char *>(mmaped) + content_length);
        ::munmap(mmaped, page_size);
        ::close(fd);
        res.flush();
        
        return true;
    }

    void read_config(std::string configfile) {
        this->_mime.read_config(configfile);
    }
};

}

#endif
