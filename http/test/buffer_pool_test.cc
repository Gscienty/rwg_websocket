#include "buffer_pool.h"
#include "gtest/gtest.h"

TEST(buffer_pool, ctor) {
    rwg_http::buffer_pool pool(128, 8);
    
    EXPECT_EQ(128, pool.unit_size());
    EXPECT_EQ(8, pool.unit_count());
}

TEST(buffer_pool, alloc) {
    rwg_http::buffer_pool pool(128, 8);

    auto buffer = pool.alloc(129);

    EXPECT_EQ(129, buffer.size());
    EXPECT_EQ(128 * 2, buffer.avail_size());

    EXPECT_TRUE(pool.map().ensure(0, 2, true));
    EXPECT_TRUE(pool.map().ensure(2, 8, false));
}

TEST(buffer_pool, alloc_and_recover) {
    rwg_http::buffer_pool pool(128, 8);
    {
        auto buffer = pool.alloc(129);
        EXPECT_TRUE(pool.map().ensure(0, 2, true));
        EXPECT_TRUE(pool.map().ensure(2, 8, false));
    }

    EXPECT_TRUE(pool.map().ensure(0, 8, false));
}

TEST(buffer_pool, alloc_and_recover_2) {
    rwg_http::buffer_pool pool(128, 8);
    auto buffer1 = pool.alloc(129);
    EXPECT_TRUE(pool.map().ensure(0, 2, true));
    EXPECT_TRUE(pool.map().ensure(2, 8, false));
    auto buffer2 = pool.alloc(129);
    EXPECT_TRUE(pool.map().ensure(0, 4, true));
    EXPECT_TRUE(pool.map().ensure(4, 8, false));
    auto buffer3 = pool.alloc(129);
    EXPECT_TRUE(pool.map().ensure(0, 6, true));
    EXPECT_TRUE(pool.map().ensure(6, 8, false));

    buffer2.recover();
    EXPECT_TRUE(pool.map().ensure(0, 2, true));
    EXPECT_TRUE(pool.map().ensure(2, 4, false));
    EXPECT_TRUE(pool.map().ensure(4, 6, true));
    EXPECT_TRUE(pool.map().ensure(6, 8, false));

    auto buffer4 = pool.alloc(128 * 3);
    EXPECT_TRUE(pool.map().ensure(0, 7, true));
    EXPECT_TRUE(pool.map().ensure(7, 8, false));
}

TEST(buffer_pool, alloc_overflow) {
    rwg_http::buffer_pool pool(128, 1);
    {
        auto buffer = pool.alloc(1);
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
    rwg_http::buffer_pool pool(128, 1);
    
    try {
        pool.alloc(129);
        FAIL();
    }
    catch (const std::bad_alloc& ex) {
        SUCCEED();
    }
}

TEST(buffer_pool, alloc_overflow_2) {
    rwg_http::buffer_pool pool(128, 2);
    
    try {
        pool.alloc(128 * 2 + 1);
        FAIL();
    }
    catch (const std::bad_alloc& ex) {
        SUCCEED();
    }
}

TEST(buffer_pool, assign_) {
    rwg_http::buffer_pool pool(128, 1);

    auto buffer = pool.alloc(128);

    auto str = "1111111111";
    buffer.assign(str, str + 11);

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ('1', buffer[i]);
    }

    EXPECT_EQ(0, buffer[10]);
}

TEST(buffer_pool, insert_) {
    rwg_http::buffer_pool pool(128, 1);

    auto buffer = pool.alloc(128);

    auto str = "1111111111";

    auto ret = buffer.insert(str, str + 11, 100);
    EXPECT_EQ(11, ret);
    
    for (int i = 100; i < 100 + 10; i++) {
        EXPECT_EQ('1', buffer[i]);
    }
    EXPECT_EQ(0, buffer[110]);
}

int main() {
    return RUN_ALL_TESTS();
}
