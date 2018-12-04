#ifndef _RWG_WEB_NOT_IMPL_STARTUP_
#define _RWG_WEB_NOT_IMPL_STARTUP_

#include <string>
#include "web/req.hpp"
#include "web/res.hpp"

namespace rwg_web {

class not_impl_startup {
public:
    void run(rwg_web::req &, rwg_web::res &);
};

}

#endif
