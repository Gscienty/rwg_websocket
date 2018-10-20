#ifndef _RWG_UTIL_BASE64_
#define _RWG_UTIL_BASE64_

#include <string>
#include <cstdint>

namespace rwg_util {

static const char __base64_byte_alpha[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '/'
};

inline std::uint8_t __base64_alpha_byte(const char c) {
    switch (c) {
    case 'A' ... 'Z':
        return c - 'A';
    case 'a' ... 'z':
        return c - 'a' + 26;
    case '0' ... '9':
        return c - '0' + 26 + 26;
    }
    if (c == '+') {
        return 30;
    }
    else {
        return 31;
    }
}

std::string base64_encode(const std::uint8_t* s, const std::size_t n) {
    std::string ret;
    std::size_t ret_size = (n + (3 - n % 3) % 3) / 3 * 4 - (3 - n % 3) % 3;
    ret.resize(ret_size);

    std::size_t byte_idx = 0;
    uint8_t bit_off = 0;
    std::size_t ret_idx = 0;

    while (ret_idx != ret_size) {
        std::uint8_t p_byte = byte_idx >= n ? 0 : s[byte_idx];
        std::uint8_t np_byte = byte_idx + 1 >= n ? 0 : s[byte_idx + 1];

        std::uint8_t b = 0;
        switch (bit_off) {
        case 0:
            b = (0xFC & p_byte) >> 2;
            bit_off = 6;
            break;
        case 1:
            b = (0x7E & p_byte) >> 1;
            bit_off = 7;
            break;
        case 2:
            b = (0x3F & p_byte);
            bit_off = 0;
            byte_idx++;
            break;
        case 3:
            b = ((0x1F & p_byte) << 1) | ((0x80 & np_byte) >> 7);
            bit_off = 1;
            byte_idx++;
            break;
        case 4:
            b = ((0x0F & p_byte) << 2) | ((0xC0 & np_byte) >> 6);
            bit_off = 2;
            byte_idx++;
            break;
        case 5:
            b = ((0x07 & p_byte) << 3) | ((0xE0 & np_byte) >> 5);
            bit_off = 3;
            byte_idx++;
            break;
        case 6:
            b = ((0x03 & p_byte) << 4) | ((0xF0 & np_byte) >> 4);
            bit_off = 4;
            byte_idx++;
            break;
        case 7:
            b = ((0x01 & p_byte) << 5) | ((0xF8 & np_byte) >> 3);
            bit_off = 5;
            byte_idx++;
            break;
        }

        ret[ret_idx++] = __base64_byte_alpha[b];
    }

    for (std::size_t i = 0; i < (3 - n % 3) % 3; i++) {
        ret.push_back('=');
    }

    return ret;
}

std::basic_string<std::uint8_t> base64_decode(const std::string& s) {
    if (s.size() % 4 != 0) {
        throw std::exception();
    }
    std::basic_string<std::uint8_t> ret;
    ret.resize(s.size() / 4 * 3);
    
    std::size_t ret_off = 0;
    std::size_t s_off = 0;
    uint8_t bit_off = 0;    

    while (s_off != s.size()) {
        if (s[s_off] == '=') {
            break;
        }

        switch (bit_off) {
        case 0:
            ret[ret_off++] = (__base64_alpha_byte(s[s_off]) << 2) | (__base64_alpha_byte(s[s_off + 1]) >> 4);
            s_off++;
            bit_off = 2;
            break;
        case 1:
            ret[ret_off++] = ((__base64_alpha_byte(s[s_off]) & 0x1F) << 3) | (__base64_alpha_byte(s[s_off + 1]) >> 3);
            s_off++;
            bit_off = 3;
            break;
        case 2:
            ret[ret_off++] = ((__base64_alpha_byte(s[s_off]) & 0x0F) << 4) | (__base64_alpha_byte(s[s_off + 1]) >> 2);
            s_off++;
            bit_off = 4;
            break;
        case 3:
            ret[ret_off++] = ((__base64_alpha_byte(s[s_off]) & 0x07) << 5) | (__base64_alpha_byte(s[s_off + 1]) >> 1);
            s_off++;
            bit_off = 5;
            break;
        case 4:
            ret[ret_off++] = ((__base64_alpha_byte(s[s_off]) & 0x03) << 6) | (__base64_alpha_byte(s[s_off + 1]));
            s_off += 2;
            bit_off = 0;
            break;
        case 5:
            ret[ret_off++] = ((__base64_alpha_byte(s[s_off]) & 0x01) << 7) | (__base64_alpha_byte(s[s_off + 1]) << 1) | ((__base64_alpha_byte(s[s_off + 2]) & 0x20) >> 5);
            s_off += 2;
            bit_off = 1;
            break;
        }
    }

    if (s.back() == '=') {
        ret.pop_back();
    }
    if (*(s.rbegin() + 1) == '=') {
        ret.pop_back();
    }

    return ret;
}

}

#endif
