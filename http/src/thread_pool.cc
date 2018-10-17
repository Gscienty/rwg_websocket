#include "thread_pool.h"
#ifdef DEBUG
#include <iostream>
#endif

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
#ifdef DEBUG
        std::cout << "thread_executor::__body: thread begin" << std::endl;
#endif
        if (this->_shutdown) {
#ifdef DEBUG
            std::cout << "thread_executor::__body: thread shutdown" << std::endl;
#endif
            break;
        }
        if (bool(this->_executor)) {
/* #ifdef DEBUG */
/*             std::cout << "thread_executor::__body: thread execute begin" << std::endl; */
/* #endif */
            this->_executor();
            this->_notify();
/* #ifdef DEBUG */
/*             std::cout << "thread_executor::__body: thread _execute end" << std::endl; */
/* #endif */
        }
        this->_trigger = false;
        lck.unlock();
        this->_cond.notify_one();
        lck.lock();
#ifdef DEBUG
        std::cout << "thread_executor::__body: thread end" << std::endl;
#endif
    }

    this->_truth_shutdown = true;
    this->_shutdown_cond.notify_one();
}

void rwg_http::thread_executor::execute(std::function<void ()> func) {
#ifdef DEBUG
    std::cout << "thread_executor::execute: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << "thread_executor::execute: begin" << std::endl;
#endif
    if (this->_shutdown) {
#ifdef DEBUG
        std::cout << "thread_executor::execute: shutdown" << std::endl;
#endif
        return;
    }
    this->_cond.wait(lck, [this] () -> bool { return this->_shutdown || this->_trigger == false; });
    this->_executor = func;
    this->_trigger = true;
    lck.unlock();

    this->_cond.notify_one();
#ifdef DEBUG
    std::cout << "thread_executor::execute: end" << std::endl;
#endif
}

void rwg_http::thread_executor::shutdown() {
#ifdef DEBUG
    std::cout << "thread_executor::shutdown: prebegin" << std::endl;
#endif
    this->_shutdown = true;
    this->_cond.notify_one();

    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_shutdown_cond.wait(lck, [this] () -> bool { return this->_truth_shutdown; });
#ifdef DEBUG
    std::cout << "thread_executor::shutdown: end" << std::endl;
#endif
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
#ifdef DEBUG
    std::cout << "thread_pool::shutdown: prebegin" << std::endl;
#endif
    this->_shutdown = true;
    this->_cond.notify_one();

    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_shutdown_cond.wait(lck, [this] () -> bool { return this->_truth_shutdown; });
#ifdef DEBUG
    std::cout << "thread_pool::shutdown: begin" << std::endl;
#endif
    // 需要等待全部任务执行结束

    for (auto instance : this->_threads_pool) {
        instance->shutdown();
        delete instance;
    }

    this->_threads_pool.clear();
    while (!this->_free.empty()) {
        this->_free.pop();
    }
#ifdef DEBUG
    std::cout << "thread_pool::shutdown: end" << std::endl;
#endif
}

void rwg_http::thread_pool::__release(int ith) {
#ifdef DEBUG
    std::cout << "thread_pool::__release: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << "thread_pool::__release: begin" << std::endl;
#endif
    this->_free.push(ith);
    lck.unlock();
    this->_cond.notify_one();
#ifdef DEBUG
    std::cout << "thread_pool::__release: end" << std::endl;
#endif
}

void rwg_http::thread_pool::__dispatch_work() {
#ifdef DEBUG
    std::cout << "thread_pool::__dispatch_work: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
    while (this->_shutdown == false) {
        this->_cond
            .wait(lck, [this] () -> bool {
                  return this->_shutdown || (!this->_tasks.empty() && !this->_free.empty());
                  });
#ifdef DEBUG
        std::cout << "thread_pool::__dispatch_work: begin" << std::endl;
#endif
        if (this->_shutdown) {
#ifdef DEBUG
            std::cout << "thread_pool::__dispatch_work: shutdown" << std::endl;
#endif
            break;
        }

        int executor_ith = this->_free.front();
        this->_free.pop();
        auto task = this->_tasks.front();
        this->_tasks.pop();

        this->_threads_pool[executor_ith]->execute(task);
    }

    this->_truth_shutdown = true;

    lck.unlock();
    this->_shutdown_cond.notify_one();
#ifdef DEBUG
    std::cout << "thread_pool::__dispatch_work: end" << std::endl;
#endif
}

void rwg_http::thread_pool::submit(std::function<void ()> work) {
#ifdef DEBUG
    std::cout << "thread_pool::submit: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << "thread_pool::submit: begin" << std::endl;
#endif
    this->_tasks.push(work);
    lck.unlock();
    this->_cond.notify_one();
#ifdef DEBUG
    std::cout << "thread_pool::submit: end" << std::endl;
#endif
}
