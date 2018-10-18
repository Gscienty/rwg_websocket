#ifndef _RWG_WEB_EVENT_
#define _RWG_WEB_EVENT_

#include <sys/epoll.h>

namespace rwg_web {

class event {
    ::epoll_event _event;
public:
    ::epoll_event& ep_event() { return this->_event; }
};

}

#endif
