#include "buf_stream.h"
#include <iostream>

rwg_http::buf_instream::buf_instream(std::size_t unit_size,
                                     std::function<std::size_t (std::uint8_t* s, std::size_t n)> sync_func,
                                     std::function<void ()> close_callback)
    : _unit_size(unit_size)
    , _using_unit(nullptr)
    , _using_unit_pos(0)
    , _using_unit_size(0)
    , _sync(sync_func)
    , _closed_flag(true)
    , _close_callback(close_callback) {
}

void rwg_http::buf_instream::set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer) {
    std::lock_guard<std::mutex> lck(this->_mtx);
    this->_closed_flag = false;
    this->_buffer = std::move(buffer);

    int unit_count = this->_buffer->size() / this->_unit_size;
    for (auto i = 0; i < unit_count; i++) {
        this->_free.push(this->_buffer->get() + (i * this->_unit_size));
    }
}

void rwg_http::buf_instream::release() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    while (!this->_free.empty()) { this->_free.pop(); }
    while (!this->_ready.empty()) { this->_free.pop(); }
    this->_closed_flag = true;
    if (bool(this->_buffer)) { 
        delete this->_buffer.release();
    }
    lck.unlock();
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
}

bool rwg_http::buf_instream::__flush() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_ready_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_ready.empty(); });
    if (this->_closed_flag) {
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

    return true;
}

void rwg_http::buf_instream::sync() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_free.empty(); });
    if (this->_closed_flag) {
        lck.unlock();
        this->_ready_cond.notify_one();
        return;
    }

    std::uint8_t* unit_ptr = this->_free.front();
    this->_free.pop();

    std::size_t size = this->_sync(unit_ptr, this->_unit_size);
    if (size == 0) {
        this->close();
        return;
    }
    this->_ready.push(std::make_pair(unit_ptr, size));

    lck.unlock();
    this->_ready_cond.notify_one();
}

void rwg_http::buf_instream::close() {
    if (this->_closed_flag) {
        return;
    }
    this->_closed_flag = true;
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
    this->_close_callback();
}

std::uint8_t rwg_http::buf_instream::getc() {
    if (this->_closed_flag) {
        return EOF;
    }
    if (this->_using_unit_pos >= this->_using_unit_size) {
        if (!this->__flush()) {
            return EOF;
        }
    }

    return this->_using_unit[this->_using_unit_pos++];
}

rwg_http::buf_outstream::buf_outstream(std::size_t unit_size,
                                       std::function<bool (std::uint8_t*, std::size_t n)> sync_func,
                                       std::function<void (rwg_http::buf_outstream&)> notify_func,
                                       std::function<void ()> close_callback)
    : _unit_size(unit_size)
    , _using_unit(nullptr)
    , _using_unit_size(0)
    , _sync(sync_func)
    , _notify(notify_func)
    , _closed_flag(true)
    , _close_callback(close_callback) {
}

void rwg_http::buf_outstream::set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer) {
    std::lock_guard<std::mutex> lck(this->_mtx);
    this->_closed_flag = false;
    this->_buffer = std::move(buffer);
    
    int unit_count = this->_buffer->size() / this->_unit_size;
    for (auto i = 0; i < unit_count; i++) {
        this->_free.push(this->_buffer->get() + (i * this->_unit_size));
    }
}

void rwg_http::buf_outstream::release() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    while (!this->_free.empty()) { this->_free.pop(); }
    while (!this->_ready.empty()) { this->_ready.pop(); }
    this->_closed_flag = true;
    if (bool(this->_buffer)) {
        delete this->_buffer.release();
    }
    lck.unlock();
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
}

bool rwg_http::buf_outstream::__flush() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_free.empty(); });
    if (this->_closed_flag) {
        lck.unlock();
        this->_ready_cond.notify_one();
        return false;
    }

    bool need_notify_sync = false;
    if (this->_using_unit != nullptr) {
        need_notify_sync = true;
        this->_ready.push(std::make_pair(this->_using_unit, this->_using_unit_size));
    }
    this->_using_unit = this->_free.front();
    this->_using_unit_size = 0;
    this->_free.pop();

    lck.unlock();
    this->_ready_cond.notify_one();

    if (need_notify_sync) {
        this->_notify(*this);
    }
    return true;
}

bool rwg_http::buf_outstream::flush() {
    if (this->_closed_flag) {
        return false;
    }
    if (this->_using_unit_size == 0) {
        return false;
    }
    if (!this->__flush()) {
        return false;
    }

    std::unique_lock<std::mutex> lck(this->_mtx);
    if (this->_ready.empty()) {
        return false;
    }
    auto unit = this->_ready.front();
    this->_ready.pop();

    if (!this->_sync(unit.first, unit.second)) {
        this->close();
        return false;
    }
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();

    lck.lock();
    this->_free_cond.wait(lck, [this] () -> bool { return this->_closed_flag || this->_ready.empty(); });
    return !this->_closed_flag;
}

void rwg_http::buf_outstream::sync() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_ready_cond.wait(lck, [this] () -> bool { return this->_closed_flag || !this->_ready.empty(); });
    while (!this->_ready.empty()) {
        if (this->_closed_flag) {
            lck.unlock();
            this->_free_cond.notify_one();
            return;
        }
        
        auto unit = this->_ready.front();
        this->_ready.pop();

        if (!this->_sync(unit.first, unit.second)) {
            this->close();
            return;
        }
        this->_free.push(unit.first);
    }

    if (!this->_free.empty()) {
        lck.unlock();
        this->_free_cond.notify_one();
    }
}

void rwg_http::buf_outstream::nonblock_sync() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    if (this->_closed_flag) {
        lck.unlock();
        this->_free_cond.notify_one();
        return;
    }
    if (this->_ready.empty()) {
        return;
    }

    auto unit = this->_ready.front();
    this->_ready.pop();

    if(!this->_sync(unit.first, unit.second)) {
        lck.unlock();
        this->_free_cond.notify_one();
        return;
    }
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();
}

void rwg_http::buf_outstream::close() {
    if (this->_closed_flag) {
        return;
    }
    this->_closed_flag = true;
    this->_free_cond.notify_one();
    this->_ready_cond.notify_one();
    this->_close_callback();
}

void rwg_http::buf_outstream::putc(std::uint8_t c) {
    if (this->_closed_flag) {
        return;
    }
    if (this->_using_unit == nullptr || this->_using_unit_size == this->_unit_size) {
        if (!this->__flush()) {
            return;
        }
    }

    this->_using_unit[this->_using_unit_size++] = c;
}
