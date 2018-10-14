#include "chain_middleware.h"

void rwg_http::chain_middleware::append(std::function<bool (rwg_http::session&)> middleware) {
    this->_middlewares.push_back(middleware);
}

void rwg_http::chain_middleware::execute(rwg_http::session& session) const {
    for (auto middleware_func : this->_middlewares) {
        if (session.closed_flag()) {
            break;
        }
        if (!middleware_func(session)) {
            break;
        }
    }
}
