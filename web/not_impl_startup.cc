#include "web/not_impl_startup.hpp"

namespace rwg_web {

void not_impl_startup::run(rwg_web::req &, rwg_web::res &res) {
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

}
