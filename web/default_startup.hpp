#ifndef _RWG_WEB_DEFAULT_STARTUP_
#define _RWG_WEB_DEFAULT_STARTUP_

#include <string>
#include "web/req.hpp"
#include "web/res.hpp"

namespace rwg_web {

class default_startup {
private:
    const std::string _uri;
public:
    default_startup(const std::string);

    void run(rwg_web::req &, rwg_web::res &);
};
}

#endif
