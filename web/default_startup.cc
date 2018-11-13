#include "web/default_startup.hpp"

namespace rwg_web {

default_startup::default_startup(const std::string uri) : _uri(uri) {}

void default_startup::run(rwg_web::req &req, rwg_web::res &) {
    if (req.uri() == "/") {
        req.uri() = this->_uri;
    }
}

}
