#include "util/sha1.hpp"

namespace rwg_util {

static inline std::uint32_t __inl_sha1_circular_shift(int bits, std::uint32_t word) {
    return (word << bits) | (word >> (32 - bits));
}

static const uint32_t __sha1_K[] = { 0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };

static void __sha1_process_block(std::uint32_t intermediate_hash[], std::uint8_t block[]) {
    std::uint32_t word[80];

    for (auto t = 0; t < 16; t++) {
        word[t] = block[t * 4] << 24;
        word[t] |= block[t * 4 + 1] << 16;
        word[t] |= block[t * 4 + 2] << 8;
        word[t] |= block[t * 4 + 3];
    }

    for (auto t = 16; t < 80; t++) {
        word[t] = __inl_sha1_circular_shift(1, word[t - 3] ^ word[t - 8] ^ word[t - 14] ^ word[t - 16]);
    }

    std::uint32_t A = intermediate_hash[0];
    std::uint32_t B = intermediate_hash[1];
    std::uint32_t C = intermediate_hash[2];
    std::uint32_t D = intermediate_hash[3];
    std::uint32_t E = intermediate_hash[4];

    for (auto t = 0; t < 20; t++) {
        std::uint32_t tmp = __inl_sha1_circular_shift(5, A) + ((B & C) | ((~B) & D)) + E + word[t] + __sha1_K[0];
        E = D;
        D = C;
        C = __inl_sha1_circular_shift(30, B);
        B = A;
        A = tmp;
    }

    for (auto t = 20; t < 40; t++) {
        std::uint32_t tmp = __inl_sha1_circular_shift(5, A) + (B ^ C ^ D) + E + word[t] + __sha1_K[1];
        E = D;
        D = C;
        C = __inl_sha1_circular_shift(30, B);
        B = A;
        A = tmp;
    }

    for (auto t = 40; t < 60; t++) {
        std::uint32_t tmp = __inl_sha1_circular_shift(5, A) + ((B & C) | (B & D) | (C & D)) + E + word[t] + __sha1_K[2];
        E = D;
        D = C;
        C = __inl_sha1_circular_shift(30, B);
        B = A;
        A = tmp;
    }

    for (auto t = 60; t < 80; t++) {
        std::uint32_t tmp = __inl_sha1_circular_shift(5, A) + (B ^ C ^ D) + E + word[t] + __sha1_K[3];
        E = D;
        D = C;
        C = __inl_sha1_circular_shift(30, B);
        B = A;
        A = tmp;
    }

    intermediate_hash[0] += A;
    intermediate_hash[1] += B;
    intermediate_hash[2] += C;
    intermediate_hash[3] += D;
    intermediate_hash[4] += E;
}

std::basic_string<uint8_t> sha1(std::basic_string<std::uint8_t> &plain) {
    std::uint32_t intermediate_hash[] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };

    std::uint8_t block[64] = { 0 };
    std::uint32_t block_index = 0;
    std::uint64_t len = 0;

    for (auto c : plain) {
        block[block_index++] = (c & 0xFF);

        len += 8;
        if (block_index == 64) {
            __sha1_process_block(intermediate_hash, block);
            block_index = 0;
        }
    }

    if (block_index > 55) {
        block[block_index++] = 0x80;
        while (block_index < 64) {
            block[block_index++] = 0;
        }

        __sha1_process_block(intermediate_hash, block);

        block_index = 0;
        while (block_index < 56) {
            block[block_index++] = 0;
        }
    }
    else {
        block[block_index++] = 0x80;
        while (block_index < 56) {
            block[block_index++] = 0x00;
        }
    }

    block[56] = len >> 56;
    block[57] = len >> 48;
    block[58] = len >> 40;
    block[59] = len >> 32;
    block[60] = len >> 24;
    block[61] = len >> 16;
    block[62] = len >> 8;
    block[63] = len;

    __sha1_process_block(intermediate_hash, block);

    std::basic_string<std::uint8_t> digest;
    digest.resize(20);

    for (auto i = 0; i < 20; i++) {
        digest[i] = intermediate_hash[i >> 2] >> 8 * (3 - (i & 0x03));
    }

    return digest;
}

}

