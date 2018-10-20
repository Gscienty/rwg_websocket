#include "sha1.hpp"
#include <iostream>
#include <iomanip>

int main() {
    std::string te1 = "abc";
    std::basic_string<std::uint8_t> te1_t(te1.begin(), te1.end());

    std::basic_string<std::uint8_t> sha1_t1 = rwg_util::sha1(te1_t);

    for (auto c : sha1_t1) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) c;
    }

    return 0;
}
