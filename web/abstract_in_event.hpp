#ifndef _RWG_WEB_ABS_IN_EVENT_
#define _RWG_WEB_ABS_IN_EVENT_

#include "web/event.hpp"

namespace rwg_web {

class abstract_in_event : public rwg_web::event {
public:
    virtual void in_event(int fd) = 0;
};

}

#endif
