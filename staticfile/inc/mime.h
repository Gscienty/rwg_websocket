#pragma once

#include <string>
#include <map>

namespace rwg_staticfile {

class mime {
private:
    std::map<std::string, std::string> _ext_mime;
    std::string _default;

public:
    mime();

    void read_config(std::string config_file);
    std::string get(std::string ext) const;
    std::string default_mime() const;
};

}
