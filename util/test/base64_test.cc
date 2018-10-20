#include "gtest/gtest.h"
#include "base64.hpp"
#include <iostream>

TEST(base64, base64_encode) {

    using namespace rwg_util;

    std::string plain("a");
    std::string base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());

    EXPECT_EQ("YQ==", base64);

    plain = "1";
    base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MQ==", base64);

    plain = "12";
    base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTI=", base64);

    plain = "123";
    base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIz", base64);

    plain = "1234";
    base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIzNA==", base64);

    plain = "12345";
    base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIzNDU=", base64);

    plain = "123456";
    base64 = base64_encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIzNDU2", base64);
}

TEST(base64, base64_decode) {
    
    using namespace rwg_util;

    std::string base64 = "YQ==";
    std::basic_string<std::uint8_t> base64_decoder = base64_decode(base64);
    std::string plain(base64_decoder.begin(), base64_decoder.end());
    EXPECT_EQ(std::string("a"), plain);

    base64 = "MQ==";
    base64_decoder = base64_decode(base64);
    plain.assign(base64_decoder.begin(), base64_decoder.end());
    EXPECT_EQ(std::string("1"), plain);

    base64 = "MTI=";
    base64_decoder = base64_decode(base64);
    plain.assign(base64_decoder.begin(), base64_decoder.end());
    EXPECT_EQ(std::string("12"), plain);

    base64 = "MTIzNA==";
    base64_decoder = base64_decode(base64);
    plain.assign(base64_decoder.begin(), base64_decoder.end());
    EXPECT_EQ(std::string("1234"), plain);
    
    base64 = "MTIzNDU=";
    base64_decoder = base64_decode(base64);
    plain.assign(base64_decoder.begin(), base64_decoder.end());
    EXPECT_EQ(std::string("12345"), plain);

    base64 = "MTIzNDU2";
    base64_decoder = base64_decode(base64);
    plain.assign(base64_decoder.begin(), base64_decoder.end());
    EXPECT_EQ(std::string("123456"), plain);

}

int main() {
    return RUN_ALL_TESTS();
}
