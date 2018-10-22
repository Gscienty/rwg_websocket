#ifndef _RWG_WEB_NOT_IMPL_STARTUP_
#define _RWG_WEB_NOT_IMPL_STARTUP_

#include <string>
#include "web/req.hpp"
#include "web/res.hpp"

namespace rwg_web {

class not_impl_startup {
public:
    void run(rwg_web::req&, rwg_web::res& res) {
        res.version() = "HTTP/1.1";
        res.status_code() = 501;
        res.description() = "Not Implemented";

        std::string html = "<!DOCTYPE html><html><head><title>Not Implemented</title></head><body><h1>Not Implemented</h1></body></html>";

        res.header_parameters()["Content-Type"] = "text/html";
        res.header_parameters()["Content-Length"] = std::to_string(html.size());

        res.write_header();

        res.write(html.begin(), html.end());
        
        res.flush();
    }
};

}

#endif
