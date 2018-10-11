#pragma once

#include <string>
#include <cstdint>

namespace rwg_util {
namespace base64 {

std::basic_string<std::uint8_t> decode(const std::string& s);

std::string encode(const std::uint8_t* s, const std::size_t n);

}
}
