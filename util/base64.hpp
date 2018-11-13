#ifndef _RWG_UTIL_BASE64_
#define _RWG_UTIL_BASE64_

#include <string>
#include <cstdint>

namespace rwg_util {

std::string base64_encode(const std::uint8_t *, const std::size_t);
std::basic_string<std::uint8_t> base64_decode(const std::string &);

}

#endif
