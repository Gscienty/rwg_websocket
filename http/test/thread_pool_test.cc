#include "thread_pool.h"
#include "gtest/gtest.h"
#include <iostream>
#include <thread>
#include <chrono>

TEST(thr, executor) {
    auto i = 1;

    auto notify = [&] () -> void {
        static auto a = 2;
        EXPECT_EQ(a++, i);
    };
    rwg_http::thread_executor executor(notify);

    auto execute = [&] () -> void {
        EXPECT_EQ(1, i);
        i++;
    };

    executor.execute(execute);

    executor.shutdown();
}

TEST(thr, pool) {
    rwg_http::thread_pool pool(1);

    auto work = [] () -> void {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Hello World\n";
    };

    pool.submit(work);
    pool.submit(work);
    pool.submit(work);

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    pool.shutdown();
}

TEST(thr, pool2) {
    rwg_http::thread_pool pool(3);

    auto work = [] () -> void {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Hello World\n";
    };

    pool.submit(work);
    pool.submit(work);
    pool.submit(work);
    pool.submit(work);

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    pool.shutdown();
}

int main() {
    return RUN_ALL_TESTS();
}
