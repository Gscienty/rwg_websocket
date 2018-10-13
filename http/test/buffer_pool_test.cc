#include "buffer_pool.h"
#include "gtest/gtest.h"

TEST(buffer_pool, alloc) {
    rwg_http::buffer_pool pool(128);

    auto buffer = pool.alloc(64);

    EXPECT_EQ(64, buffer.size());
    EXPECT_EQ(1, pool.usable().size());
}

TEST(buffer_pool, alloc_and_recover) {
    rwg_http::buffer_pool pool(128);
    {
        auto buffer = pool.alloc(65);
        EXPECT_EQ(65, buffer.size());
        EXPECT_TRUE(pool.usable().empty());
    }
    EXPECT_FALSE(pool.usable().empty());
}

TEST(buffer_pool, alloc_and_recover_2) {
    rwg_http::buffer_pool pool(128);

    auto buffer1 = pool.alloc(2);
    auto buffer2 = pool.alloc(4);
    auto buffer3 = pool.alloc(8);

    EXPECT_EQ(3, pool.unusable().size());

    buffer1.recover();
    EXPECT_EQ(0, pool.usable().front().first);
    EXPECT_EQ(4, pool.usable().front().second);

    buffer2.recover();
    EXPECT_EQ(0, pool.usable().front().first);
    EXPECT_EQ(8, pool.usable().front().second);

    buffer3.recover();
    EXPECT_EQ(0, pool.usable().front().first);
    EXPECT_EQ(128, pool.usable().front().second);
}

TEST(buffer_pool, alloc_and_recover_3) {
    rwg_http::buffer_pool pool(128);

    auto buffer1 = pool.alloc(2);
    auto buffer2 = pool.alloc(2);
    auto buffer3 = pool.alloc(8);

    EXPECT_EQ(4, pool.usable().front().first);
    EXPECT_EQ(8, pool.usable().front().second);

    buffer2.recover();
    EXPECT_EQ(2, pool.usable().front().first);
    EXPECT_EQ(4, pool.usable().front().second);

    auto buffer4 = pool.alloc(6);
    EXPECT_EQ(2, pool.usable().front().first);
    EXPECT_EQ(4, pool.usable().front().second);

    auto buffer5 = pool.alloc(2);
    EXPECT_EQ(4, pool.usable().front().first);
    EXPECT_EQ(8, pool.usable().front().second);

    auto buffer6 = pool.alloc(2);
    EXPECT_EQ(6, pool.usable().front().first);
    EXPECT_EQ(8, pool.usable().front().second);

    auto buffer7 = pool.alloc(2);
    buffer5.recover();
    buffer6.recover();
    EXPECT_EQ(2, pool.usable().front().first);
    EXPECT_EQ(6, pool.usable().front().second);
}

TEST(buffer_pool, alloc_overflow) {
    rwg_http::buffer_pool pool(128);
    {
        auto buffer = pool.alloc(65);
        try {
            pool.alloc(1);
            FAIL();
        }
        catch (const std::bad_alloc& ex) {
            SUCCEED();
        }
    }
    try {
        pool.alloc(1);
        SUCCEED();
    }
    catch (const std::bad_alloc& ex) {
        FAIL();
    }
}

TEST(buffer_pool, alloc_overflow_1) {
    rwg_http::buffer_pool pool(128);
    
    try {
        pool.alloc(129);
        FAIL();
    }
    catch (const std::bad_alloc& ex) {
        SUCCEED();
    }
}

TEST(buffer_pool, assign_) {
    rwg_http::buffer_pool pool(128);

    auto buffer = pool.alloc(128);

    auto str = "1111111111";
    buffer.assign(str, str + 11);

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ('1', buffer[i]);
    }

    EXPECT_EQ(0, buffer[10]);
}

TEST(buffer_pool, insert_) {
    rwg_http::buffer_pool pool(128);

    auto buffer = pool.alloc(128);

    auto str = "1111111111";

    auto ret = buffer.insert(str, str + 11, 100);
    EXPECT_EQ(11, ret);
    
    for (int i = 100; i < 100 + 10; i++) {
        EXPECT_EQ('1', buffer[i]);
    }
    EXPECT_EQ(0, buffer[110]);
}

TEST(buffer_pool, copy_to) {
    rwg_http::buffer_pool pool(128);
    auto buffer = pool.alloc(128);
    auto str = "1111111111";
    buffer.assign(str, str + 11);

    std::uint8_t ret[11];
    buffer.copy_to(0, 11, ret);

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ('1', ret[i]);
    }
    EXPECT_EQ(0, ret[10]);
}

int main() {
    return RUN_ALL_TESTS();
}
