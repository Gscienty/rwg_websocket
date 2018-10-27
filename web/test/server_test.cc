#include "web/server.hpp"

int main() {
    rwg_web::server server(10, 1, 2000);
    server.listen("127.0.0.1", 8088);
    auto exec = [] (rwg_web::req&, rwg_web::res& res, std::function<void ()>) -> void {
        res.version() = "HTTP/1.1";
        res.status_code() = 200;
        res.description() = "OK";

        res.header_parameters()["Content-Type"] = "plain/html";
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
    server.http_handle(exec);
    server.start();

    return 0;
}
