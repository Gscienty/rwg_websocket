#include "buf_stream.h"
#ifdef DEBUG
#include <iostream>
#endif

rwg_http::buf_instream::buf_instream(std::size_t unit_size,
                                     std::function<std::size_t (std::uint8_t* s, std::size_t n)> sync_func,
                                     std::function<void ()> close_callback,
                                     int fd)
    : _unit_size(unit_size)
    , _using_unit(nullptr)
    , _using_unit_pos(0)
    , _using_unit_size(0)
    , _sync(sync_func)
    , _closed_flag(true)
    , _close_callback(close_callback)
    , _log_fd(fd) {
}

void rwg_http::buf_instream::set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer) {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::set_buffer: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::set_buffer: begin" << std::endl;
#endif
    this->_closed_flag = false;
    this->_buffer = std::move(buffer);

    int unit_count = this->_buffer->size() / this->_unit_size;
    for (auto i = 0; i < unit_count; i++) {
        this->_free.push(this->_buffer->get() + (i * this->_unit_size));
    }
    this->_using_unit_pos = 0;
    this->_using_unit_size = 0;
    this->_using_unit = nullptr;
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::set_buffer: end" << std::endl;
#endif
}

void rwg_http::buf_instream::release() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::release: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::release: begin" << std::endl;
#endif
    while (!this->_free.empty()) { this->_free.pop(); }
    while (!this->_ready.empty()) { this->_ready.pop(); }
    if (bool(this->_buffer)) { 
        this->_buffer.reset(nullptr);
    }
    this->_using_unit_pos = 0;
    this->_using_unit_size = 0;
    this->_using_unit = nullptr;
    lck.unlock();
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
#ifdef DEBUG
    std::cout<< this->_log_fd << " buf_instream::release: end" << std::endl;
#endif
}

bool rwg_http::buf_instream::__flush() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::__flush: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_ready_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_ready.empty(); });
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::__flush: begin" << std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_instream::__flush: closed" << std::endl;
#endif
        lck.unlock();
        this->_free_cond.notify_one();
        return false;
    }

    if (this->_using_unit != nullptr) {
        this->_free.push(this->_using_unit);
    }
    auto ret = this->_ready.front();
    this->_ready.pop();

    this->_using_unit = ret.first;
    this->_using_unit_pos = 0;
    this->_using_unit_size = ret.second;

    lck.unlock();
    this->_free_cond.notify_one();

#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::__flush: end" << std::endl;
#endif
    return true;
}

void rwg_http::buf_instream::sync() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::sync: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_free.empty(); });
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::sync: begin" << std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_instream::sync: closed" << std::endl;
#endif
        lck.unlock();
        this->_ready_cond.notify_one();
        return;
    }

    if (this->_free.empty()) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_instream::sync: free queue empty" << std::endl;
#endif
        this->close();
        return;
    }
    std::uint8_t* unit_ptr = this->_free.front();
    this->_free.pop();

    std::size_t size = this->_sync(unit_ptr, this->_unit_size);
    if (size == 0) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_instream::sync: sync result 0" << std::endl;
#endif
        this->close();
        return;
    }
    this->_ready.push(std::make_pair(unit_ptr, size));

    lck.unlock();
    this->_ready_cond.notify_one();
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::sync: end" << std::endl;
#endif
}

void rwg_http::buf_instream::close() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::close: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::close: begin" << std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_instream::close: closed" << std::endl;
#endif
        return;
    }
    this->_closed_flag = true;
    lck.unlock();

    this->release();
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
    this->_close_callback();
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_instream::close: end" << std::endl;
#endif
}

std::uint8_t rwg_http::buf_instream::getc() {
/* #ifdef DEBUG */
/*     std::cout << "buf_instream::getc: begin" << std::endl; */
/* #endif */
    if (this->_closed_flag) {
/* #ifdef DEBUG */
/*         std::cout << "buf_instream::getc: closed" << std::endl; */
/* #endif */
        return EOF;
    }
    if (this->_using_unit_pos >= this->_using_unit_size) {
        if (!this->__flush()) {
/* #ifdef DEBUG */
/*             std::cout << "buf_instream::getc: __flush error" << std::endl; */
/* #endif */
            return EOF;
        }
    }

/* #ifdef DEBUG */
/*     std::cout << "buf_instream::getc: end" << std::endl; */
/* #endif */
    return this->_using_unit[this->_using_unit_pos++];
}

rwg_http::buf_outstream::buf_outstream(std::size_t unit_size,
                                       std::function<bool (std::uint8_t*, std::size_t n)> sync_func,
                                       std::function<void (rwg_http::buf_outstream&)> notify_func,
                                       std::function<void ()> close_callback,
                                       int fd)
    : _unit_size(unit_size)
    , _using_unit(nullptr)
    , _using_unit_size(0)
    , _sync(sync_func)
    , _notify(notify_func)
    , _closed_flag(true)
    , _close_callback(close_callback)
    , _log_fd(fd) {
}

void rwg_http::buf_outstream::set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer) {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::set_buffer: prebegin" << std::endl;
#endif
    std::lock_guard<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::set_buffer: begin" << std::endl;
#endif
    this->_closed_flag = false;
    this->_buffer = std::move(buffer);
    
    int unit_count = this->_buffer->size() / this->_unit_size;
    for (auto i = 0; i < unit_count; i++) {
        this->_free.push(this->_buffer->get() + (i * this->_unit_size));
    }
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::set_buffer: end" << std::endl;
#endif
}

void rwg_http::buf_outstream::release() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::release: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::release: begin" << std::endl;
#endif
    while (!this->_free.empty()) { this->_free.pop(); }
    while (!this->_ready.empty()) { this->_ready.pop(); }
    if (bool(this->_buffer)) {
        this->_buffer.reset(nullptr);
    }
    lck.unlock();
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::release: end" << std::endl;
#endif
}

bool rwg_http::buf_outstream::__flush() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::__flush: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_free.empty(); });
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::__flush: begin"<< std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::__flush: closed" << std::endl;
#endif
        lck.unlock();
        this->_ready_cond.notify_one();
        return false;
    }

    bool need_notify_sync = false;
    if (this->_using_unit != nullptr) {
        need_notify_sync = true;
        this->_ready.push(std::make_pair(this->_using_unit, this->_using_unit_size));
    }

    if (this->_free.empty()) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::__flush: free empty" << std::endl;
#endif
        lck.unlock();
        this->_ready_cond.notify_one();
        return false;
    }
    this->_using_unit = this->_free.front();
    this->_using_unit_size = 0;
    this->_free.pop();

    lck.unlock();
    this->_ready_cond.notify_one();

    if (need_notify_sync) {
        this->_notify(*this);
    }
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::__flush: end" << std::endl;
#endif
    return true;
}

bool rwg_http::buf_outstream::flush() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::flush: prebegin" << std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::flush: closed" << std::endl;
#endif
        return false;
    }
    if (this->_using_unit_size == 0) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::flush: unit_size 0" << std::endl;
#endif
        return false;
    }
    if (!this->__flush()) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::flush: __flush false" << std::endl;
#endif
        return false;
    }

    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::flush: begin" << std::endl;
#endif
    if (this->_ready.empty()) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::flush: ready empty" << std::endl;
#endif
        return false;
    }
    auto unit = this->_ready.front();
    this->_ready.pop();

    if (!this->_sync(unit.first, unit.second)) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::flush: sync false" << std::endl;
#endif
        this->close();
        return false;
    }
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();

    lck.lock();
    this->_free_cond.wait(lck, [this] () -> bool { return this->_closed_flag || this->_ready.empty(); });
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::flush: end" << std::endl;
#endif
    return !this->_closed_flag;
}

void rwg_http::buf_outstream::sync() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::sync: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_ready_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_ready.empty(); });
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::sync: begin" << std::endl;
#endif
    while (!this->_ready.empty()) {
        if (this->_closed_flag) {
#ifdef DEBUG
            std::cout << this->_log_fd << " buf_outstream::sync: closed" << std::endl;
#endif
            lck.unlock();
            this->_free_cond.notify_one();
            return;
        }
        
        auto unit = this->_ready.front();
        this->_ready.pop();

        if (!this->_sync(unit.first, unit.second)) {
#ifdef DEBUG
            std::cout << this->_log_fd << " buf_outstream::sync: _sync false" << std::endl;
#endif
            this->close();
            return;
        }
        this->_free.push(unit.first);
    }

    if (!this->_free.empty()) {
        lck.unlock();
        this->_free_cond.notify_one();
    }
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::sync: end" << std::endl;
#endif
}

void rwg_http::buf_outstream::nonblock_sync() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::nonblock_sync: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::nonblock_sync: begin" << std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::nonblock_sync: closed" << std::endl;
#endif
        lck.unlock();
        this->_free_cond.notify_one();
        return;
    }
    if (this->_ready.empty()) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::nonblock_sync: ready empty" << std::endl;
#endif
        return;
    }

    auto unit = this->_ready.front();
    this->_ready.pop();

    if(!this->_sync(unit.first, unit.second)) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::nonblock_sync: _sync false" << std::endl;
#endif
        lck.unlock();
        this->_free_cond.notify_one();
        return;
    }
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::nonblock_sync: end" << std::endl;
#endif
}

void rwg_http::buf_outstream::close() {
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::close: prebegin" << std::endl;
#endif
    std::unique_lock<std::mutex> lck(this->_mtx);
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::close: begin" << std::endl;
#endif
    if (this->_closed_flag) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_outstream::close: closed" << std::endl;
#endif
        return;
    }
    this->_closed_flag = true;
    lck.unlock();
    this->release();
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
    this->_close_callback();
#ifdef DEBUG
    std::cout << this->_log_fd << " buf_outstream::close: end" << std::endl;
#endif
}

void rwg_http::buf_outstream::putc(std::uint8_t c) {
/* #ifdef DEBUG */
/*     std::cout << this->_log_fd << " buf_outstream::putc: begin" << std::endl; */
/* #endif */
    if (this->_closed_flag) {
/* #ifdef DEBUG */
/*         std::cout << this->_log_fd << " buf_outstream::putc: closed" << std::endl; */
/* #endif */
        return;
    }
    if (this->_using_unit == nullptr || this->_using_unit_size == this->_unit_size) {
        if (!this->__flush()) {
/* #ifdef DEBUG */
/*             std::cout << this->_log_fd << " buf_outstream::put: __flush false" << std::endl; */
/* #endif */
            return;
        }
    }

    this->_using_unit[this->_using_unit_size++] = c;
/* #ifdef DEBUG */
/*     std::cout << this->_log_fd << " buf_outstream::put: end" << std::endl; */
/* #endif */
}
