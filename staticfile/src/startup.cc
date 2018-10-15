#include "staticfile_startup.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

rwg_staticfile::startup::startup(std::string start, std::string path)
    : _start(start)
    , _path(path) {

}

std::string rwg_staticfile::startup::__realpath(std::string uri) {
    return this->_path + uri.substr(this->_start.size());
}

bool rwg_staticfile::startup::run(rwg_http::session& session) {
    if (session.req().uri().find(this->_start) != 0) {
        return true;
    }
    std::string realpath = this->__realpath(session.req().uri());
    if (::access(realpath.c_str(), F_OK) != 0 || ::access(realpath.c_str(), R_OK) != 0) {
        this->__notfound_response(session);
        return false;
    }
    
    int fd = ::open(realpath.c_str(), O_RDONLY);
    std::size_t content_length = this->__content_length(fd);

    session.res().header_parameters()["Content-Type"] = this->__content_type(session.req().uri());
    session.res().header_parameters()["Content-Length"] = std::to_string(content_length);
    session.res().write_header();

    std::size_t page_size = ((content_length / _SC_PAGESIZE) + (content_length % _SC_PAGESIZE == 0 ? 0 : 1)) * _SC_PAGESIZE;

    void* mmaped = ::mmap(NULL, page_size, PROT_READ, MAP_SHARED, fd, 0);
    session.res().write(reinterpret_cast<std::uint8_t*>(mmaped), content_length);
    session.res().flush();
    ::munmap(mmaped, page_size);
    ::close(fd);
    return false;
}

void rwg_staticfile::startup::read_config(std::string config_file) {
    if (::access(config_file.c_str(), F_OK) == 0) {
        this->_mime.read_config(config_file);
    }
}

void rwg_staticfile::startup::__notfound_response(rwg_http::session& session) {
    session.res().status_code() = 404;
    session.res().description() = "Not Found";

    session.res().write_header();
    session.res().flush();
}

std::string rwg_staticfile::startup::__content_type(std::string uri) {
    std::size_t ext_dot = uri.rfind('.');
    if (ext_dot == std::string::npos) {
        return this->_mime.default_mime();
    }
    else {
        std::string ext = uri.substr(ext_dot + 1);
        return this->_mime.get(ext);
    }
}

std::size_t rwg_staticfile::startup::__content_length(int fd) {
    ::lseek(fd, 0, SEEK_END);
    std::size_t size = ::lseek(fd, 0, SEEK_CUR);
    ::lseek(fd, 0, SEEK_SET);

    return size;
}
