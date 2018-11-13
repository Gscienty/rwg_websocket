#include "staticfile/mime.hpp"

namespace rwg_staticfile {
mime::mime() {}

void mime::read_config(std::string config_file) {
    this->_ext_mime.clear();

    std::fstream file(config_file);
    std::string line;
    while(std::getline(file, line)) {
        std::size_t pos = line.find(' ');
        if (pos == std::string::npos) {
            continue;
        }

        std::string ext = line.substr(0, pos);
        std::string mime = line.substr(pos + 1);

        this->_ext_mime[ext] = mime;
    }

    if (this->_ext_mime.find("__default") == this->_ext_mime.end()) {
        this->_default = "application/octet-stream";
    }
    else {
        this->_default = this->_ext_mime["__default"];
    }
}

std::string mime::get(std::string ext) const {
    auto itr = this->_ext_mime.find(ext);
    if (itr == this->_ext_mime.end()) {
        return this->_default;
    }
    return itr->second;
}

std::string mime::default_mime() const {
    return this->_default;
}

}
