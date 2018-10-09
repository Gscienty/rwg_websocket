#include "bitmap.h"
#include "gtest/gtest.h"

TEST(bitmap, ctor) {
    rwg_http::bitmap bitmap_1(7);
    EXPECT_EQ(7, bitmap_1.size());
    EXPECT_EQ(1, bitmap_1.units_size());

    rwg_http::bitmap bitmap_2(8);
    EXPECT_EQ(8, bitmap_2.size());
    EXPECT_EQ(1, bitmap_2.units_size());

    rwg_http::bitmap bitmap_3(9);
    EXPECT_EQ(9, bitmap_3.size());
    EXPECT_EQ(2, bitmap_3.units_size());

    rwg_http::bitmap bitmap_4(15);
    EXPECT_EQ(15, bitmap_4.size());
    EXPECT_EQ(2, bitmap_4.units_size());

    rwg_http::bitmap bitmap_5(16);
    EXPECT_EQ(16, bitmap_5.size());
    EXPECT_EQ(2, bitmap_5.units_size());
}

TEST(bitmap, get_out_of_range) {
    rwg_http::bitmap bitmap(8);
    try {
        bitmap[9];
        FAIL();
    }
    catch (std::out_of_range& ex) {
        SUCCEED();
    }

    try {
        bitmap[8];
        FAIL();
    }
    catch (std::out_of_range& ex) {
        SUCCEED();
    }

    try {
        bitmap[7];
        SUCCEED();
    }
    catch (std::out_of_range& ex) {
        FAIL();
    }

    try {
        bitmap[0];
        SUCCEED();
    }
    catch (std::out_of_range& ex) {
        FAIL();
    }

    try {
        bitmap[-1];
        FAIL();
    }
    catch (std::out_of_range& ex) {
        SUCCEED();
    }
}

TEST(bitmap, get_default) {
    rwg_http::bitmap bitmap(8);

    EXPECT_FALSE(bitmap[7]);
}

TEST(bitmap, change) {
    rwg_http::bitmap bitmap(8);

    bitmap[7] = true;
    EXPECT_TRUE(bitmap[7]);
    for (auto i = 0; i < 7; i++) {
        EXPECT_FALSE(bitmap[i]);
    }

    bitmap[7] = false;
    bitmap[4] = true;
    for (auto i = 0; i < 4; i++) {
        EXPECT_FALSE(bitmap[i]);
    }
    EXPECT_TRUE(bitmap[4]);
    for (auto i = 5; i < 8; i++) {
        EXPECT_FALSE(bitmap[i]);
    }
}

TEST(bitmap, fill) {
   rwg_http::bitmap bitmap(80);

   bitmap.fill(10, 20, true);
   bitmap.fill(50, 80, true);
   bitmap.fill(60, 70, false);

   EXPECT_TRUE(bitmap.ensure(0, 10, false));
   EXPECT_TRUE(bitmap.ensure(10, 20, true));
   EXPECT_TRUE(bitmap.ensure(20, 50, false));
   EXPECT_TRUE(bitmap.ensure(50, 60, true));
   EXPECT_TRUE(bitmap.ensure(60, 70, false));
   EXPECT_TRUE(bitmap.ensure(70, 80, true));

   EXPECT_FALSE(bitmap.ensure(0, 20, true));
   EXPECT_FALSE(bitmap.ensure(0, 20, false));

   bitmap.fill(0, 80, true);
   EXPECT_TRUE(bitmap.ensure(0, 80, true));
   bitmap[40] = false;
   EXPECT_FALSE(bitmap.ensure(0, 80, true));

   bitmap.fill(0, 10, false);
   bitmap.fill(0, 2, true);
   EXPECT_TRUE(bitmap.ensure(0, 2, true));
}

TEST(bitmap, one_byte) {
    rwg_http::bitmap bitmap(8);

    bitmap.fill(0, 2, true);
    bitmap.fill(5, 8, true);

    EXPECT_TRUE(bitmap.ensure(0, 2, true));
    EXPECT_TRUE(bitmap.ensure(2, 5, false));
    EXPECT_TRUE(bitmap.ensure(5, 8, true));
}

int main() {
    return RUN_ALL_TESTS();
}
