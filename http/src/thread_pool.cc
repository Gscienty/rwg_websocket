#include "thread_pool.h"
#include <iostream>

rwg_http::thread_executor::thread_executor(std::function<void ()> notify)
    : _thr(std::bind(&rwg_http::thread_executor::__body, this))
    , _notify(notify) 
    , _shutdown(false)
    , _truth_shutdown(false)
    , _trigger(false) {
    this->_thr.detach();
}

void rwg_http::thread_executor::__body() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    while (this->_shutdown == false) {
        this->_cond.wait(lck, [this] () -> bool { return this->_shutdown || this->_trigger; });
        if (this->_shutdown) { break; }
        if (bool(this->_executor)) {
            this->_executor();
            this->_notify();
        }
        this->_trigger = false;
    }

    this->_truth_shutdown = true;
    lck.unlock();
    this->_shutdown_cond.notify_one();
}

void rwg_http::thread_executor::execute(std::function<void ()> func) {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_executor = func;
    this->_trigger = true;
    lck.unlock();

    this->_cond.notify_one();
}

void rwg_http::thread_executor::shutdown() {
    this->_shutdown = true;
    this->_cond.notify_one();

    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_shutdown_cond.wait(lck, [this] () -> bool { return this->_truth_shutdown; });
}

rwg_http::thread_pool::thread_pool(int size)
    : _shutdown(false)
    , _truth_shutdown(false) {

    for (auto i = 0; i <= size; i++) {
        this->_free.push(i);
        using namespace rwg_http;
        this->_threads_pool
            .push_back(new thread_executor(std::bind(&thread_pool::__release,
                                                     this,
                                                     i)));
    }
    
    auto master = this->_free.front();
    this->_free.pop();

    this->_threads_pool[master]->execute(std::bind(&rwg_http::thread_pool::__dispatch_work, this));
}

void rwg_http::thread_pool::shutdown() {
    this->_shutdown = true;
    this->_cond.notify_one();

    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_shutdown_cond.wait(lck, [this] () -> bool { return this->_truth_shutdown; });
    // 需要等待全部任务执行结束

    for (auto instance : this->_threads_pool) {
        instance->shutdown();
        delete instance;
    }

    this->_threads_pool.clear();
    while (!this->_free.empty()) {
        this->_free.pop();
    }
}

void rwg_http::thread_pool::__release(int ith) {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free.push(ith);
    lck.unlock();
    this->_cond.notify_one();
}

void rwg_http::thread_pool::__dispatch_work() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    while (this->_shutdown == false) {
        this->_cond
            .wait(lck, [this] () -> bool {
                  return this->_shutdown || (!this->_tasks.empty() && !this->_free.empty());
                  });
        if (this->_shutdown) { break; }

        int executor_ith = this->_free.front();
        this->_free.pop();
        auto task = this->_tasks.front();
        this->_tasks.pop();

        this->_threads_pool[executor_ith]->execute(task);
    }

    this->_truth_shutdown = true;
    lck.unlock();

    this->_shutdown_cond.notify_one();
}

void rwg_http::thread_pool::submit(std::function<void ()> work) {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_tasks.push(work);
    lck.unlock();
    this->_cond.notify_one();
}
