#include "web/server.hpp"
#include <iostream>

int main() {
    using namespace rwg_web;

    server ser;

    ser.use_security();
    ser.init_ssl("./server.crt", "./server.key");

    auto http_handle = [] (rwg_web::req &, rwg_web::res &res, std::function<void ()>) -> void {
        res.version() = "HTTP/1.1";
        res.status_code() = 200;
        res.description() = "OK";

        res.header_parameters()["Content-Type"] = "text/html";
        std::string html = "\
                            <!DOCTYPE html>\
                            <html>\
                            <head>\
                            <title>Test Page</title>\
                            </head>\
                            <body>\
                            <h1>Hello World</h1>\
                            <p>This is a test page.</p>\
                            </body>\
                            </html>\
                            ";
        res.header_parameters()["Content-Length"] = std::to_string(html.size());
        res.header_parameters()["Connection"] = "keep-alive";

        res.write_header();
        res.write(html.begin(), html.end());
        res.flush();

    };

    ser.http_handle(http_handle);

    ser.listen("0.0.0.0", 5000);
    ser.start();

    return 0;
}
