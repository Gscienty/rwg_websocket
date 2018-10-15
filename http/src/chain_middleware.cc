#include "chain_middleware.h"

void rwg_http::chain_middleware::append(std::function<bool (rwg_http::session&)> middleware) {
    this->_middlewares.push_back(middleware);
}

void rwg_http::chain_middleware::execute(rwg_http::session& session) const {
    if (!session.closed_flag()) {
        session.req().clear();
        session.res().clear();

        session.req().get_header();
        
        bool handled = false;
        for (auto middleware_func : this->_middlewares) {
            if (session.closed_flag()) {
                break;
            }
            if (!middleware_func(session)) {
                handled = true;
                break;
            }
        }

        if (handled == false) {
            session.res().status_code() = 400;
            session.res().description() = "Bad Request";
            session.res().write_header();
            session.res().flush();
        }
    }
}
