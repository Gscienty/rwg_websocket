#ifndef _RWG_WEB_ABSTRACT_IN_EVENT_
#define _RWG_WEB_ABSTRACT_IN_EVENT_

#include "web/event.hpp"

namespace rwg_web {

class abstract_in_event : public rwg_web::event {
public:
    virtual ~abstract_in_event();
    virtual void in_event() = 0;
};

}

#endif
