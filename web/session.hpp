#ifndef _RWG_WEB_SESSION_
#define _RWG_WEB_SESSION_

#include "web/abstract_in_event.hpp"
#include "web/res.hpp"
#include "web/req.hpp"
#include <functional>
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_web {

class session : public rwg_web::abstract_in_event {
private:
    std::function<void (rwg_web::req&, rwg_web::res&)> _executor;
public:
    virtual void in_event(int fd) override {
#ifdef DEBUG
        std::cout << "session received message" << std::endl;
#endif
        rwg_web::req req(fd);
        rwg_web::res res(fd);
        if (bool(this->_executor)) {
#ifdef DEBUG
            std::cout << "execute action" << std::endl;
#endif
            this->_executor(req, res);
        }
    }

    void run(std::function<void (rwg_web::req&, rwg_web::res&)> executor) {
        this->_executor = executor;
    }
};

}

#endif
