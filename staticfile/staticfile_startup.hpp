#ifndef _RWG_STATICFILE_STARTUP_
#define _RWG_STATICFILE_STARTUP_

#include "staticfile/mime.hpp"
#include "web/req.hpp"
#include "web/res.hpp"
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_staticfile {

class startup {
private:
    std::string _start;
    std::string _path;

    rwg_staticfile::mime _mime;

    void __notfound(rwg_web::req &, rwg_web::res &);
    std::size_t __content_length(int);
    std::string __content_type(std::string);
    std::string __realpath(std::string);

public:
    startup(std::string, std::string);

    bool run(rwg_web::req &, rwg_web::res &);
    void read_config(std::string);
};

}

#endif
