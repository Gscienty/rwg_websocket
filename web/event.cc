#include "web/event.hpp"

namespace rwg_web {

event::event() {
    this->_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP; 
}

event::~event() {}

::epoll_event& event::ep_event() {
    return this->_event;
}

}
