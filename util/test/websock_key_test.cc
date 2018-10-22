#include "util/base64.hpp"
#include "util/sha1.hpp"

#include <string>
#include <iostream>
#include <iomanip>

int main() {
    /* std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; */
    std::string key = "sN9cRrP/n9NdMgdcy2VJFQ==";
    std::string uid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    std::basic_string<uint8_t> p(key.begin(), key.end());
    p.append(uid.begin(), uid.end());
    std::basic_string<uint8_t> k = rwg_util::sha1(p);

    for (auto c : k) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int) c;
    }

    std::cout << std::endl;

    std::string b = rwg_util::base64_encode(k.data(), k.size());

    std::cout << b << std::endl;

    return 0;
}
