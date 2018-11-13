#ifndef _RWG_STATICFILE_MIME_
#define _RWG_STATICFILE_MIME_

#include <string>
#include <map>
#include <fstream>

namespace rwg_staticfile {

class mime {
private:
    std::map<std::string, std::string> _ext_mime;
    std::string _default;

public:
    mime();

    void read_config(std::string);
    std::string get(std::string) const;
    std::string default_mime() const;
};

}

#endif
