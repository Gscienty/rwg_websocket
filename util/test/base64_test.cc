#include "gtest/gtest.h"
#include "base64.h"
#include <iostream>

TEST(base64, encode) {

    using namespace rwg_util::base64;

    std::string plain("a");
    std::string base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());

    EXPECT_EQ("YQ==", base64);

    plain = "1";
    base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MQ==", base64);

    plain = "12";
    base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTI=", base64);

    plain = "123";
    base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIz", base64);

    plain = "1234";
    base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIzNA==", base64);

    plain = "12345";
    base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIzNDU=", base64);

    plain = "123456";
    base64 = encode(reinterpret_cast<const std::uint8_t*>(plain.data()), plain.size());
    EXPECT_EQ("MTIzNDU2", base64);
}

TEST(base64, decode) {
    
    using namespace rwg_util::base64;

    std::string base64 = "YQ==";
    std::basic_string<std::uint8_t> decoder = decode(base64);
    std::string plain(decoder.begin(), decoder.end());
    EXPECT_EQ(std::string("a"), plain);

    base64 = "MQ==";
    decoder = decode(base64);
    plain.assign(decoder.begin(), decoder.end());
    EXPECT_EQ(std::string("1"), plain);

    base64 = "MTI=";
    decoder = decode(base64);
    plain.assign(decoder.begin(), decoder.end());
    EXPECT_EQ(std::string("12"), plain);

    base64 = "MTIzNA==";
    decoder = decode(base64);
    plain.assign(decoder.begin(), decoder.end());
    EXPECT_EQ(std::string("1234"), plain);
    
    base64 = "MTIzNDU=";
    decoder = decode(base64);
    plain.assign(decoder.begin(), decoder.end());
    EXPECT_EQ(std::string("12345"), plain);

    base64 = "MTIzNDU2";
    decoder = decode(base64);
    plain.assign(decoder.begin(), decoder.end());
    EXPECT_EQ(std::string("123456"), plain);

}

int main() {
    return RUN_ALL_TESTS();
}
