#ifndef _RWG_UTIL_SHA1_
#define _RWG_UTIL_SHA1_

#include <string>
#include <cstdint>

namespace rwg_util {

std::basic_string<uint8_t> sha1(std::basic_string<std::uint8_t> &); 

}

#endif
