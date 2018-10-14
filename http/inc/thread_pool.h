#pragma once

#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>

namespace rwg_http {

class thread_executor {
private:
    std::thread _thr;
    std::function<void ()> _executor;
    std::function<void ()> _notify;

    bool _shutdown;
    bool _truth_shutdown;
    bool _trigger;

    std::mutex _mtx;
    std::condition_variable _cond;
    std::condition_variable _shutdown_cond;

    void __body();
public:
    thread_executor(std::function<void ()> notify);
    void execute(std::function<void ()> func);
    void shutdown();
};

class thread_pool {
private:
    std::vector<rwg_http::thread_executor*> _threads_pool;
    std::queue<std::function<void ()>> _tasks;
    std::queue<int> _free;

    std::mutex _mtx;
    std::condition_variable _cond;
    std::condition_variable _shutdown_cond;

    bool _shutdown;
    bool _truth_shutdown;

    void __release(int ith);
    void __dispatch_work();
public:
    thread_pool(int size);
    void submit(std::function<void ()> task);
    void shutdown();
};

}
