#pragma once

#include "session.h"
#include "mime.h"

namespace rwg_staticfile {

class startup {
private:
    std::string _start;
    std::string _path;

    rwg_staticfile::mime _mime;

    std::string __realpath(std::string uri);
    void __notfound_response(rwg_http::session& session);
    std::string __content_type(std::string uri);
    std::size_t __content_length(int fd);
public:
    startup(std::string start, std::string path);
    bool run(rwg_http::session& session);
    void read_config(std::string config_file);
};

}
