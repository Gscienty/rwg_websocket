#include "buf_stream.h"
#include <iostream>

rwg_http::buf_instream::buf_instream(rwg_http::buffer&& buffer,
                                     std::size_t unit_size,
                                     std::function<std::size_t (std::uint8_t* s, std::size_t n)> sync_func)
    : _buffer(std::move(buffer))
    , _unit_size(unit_size)
    , _using_unit(nullptr)
    , _using_unit_pos(0)
    , _using_unit_size(0)
    , _sync(sync_func) {

    int unit_count = this->_buffer.size() / unit_size;
    for (auto i = 0; i < unit_count; i++) {
        this->_free.push(this->_buffer.get() + (i * unit_size));
    }
}

void rwg_http::buf_instream::__flush() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_ready_cond.wait(lck, [this] () -> bool { return !this->_ready.empty(); });

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

}

void rwg_http::buf_instream::sync() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free_cond.wait(lck, [this] () -> bool { return !this->_free.empty(); });

    std::uint8_t* unit_ptr = this->_free.front();
    this->_free.pop();

    std::size_t size = this->_sync(unit_ptr, this->_unit_size);
    this->_ready.push(std::make_pair(unit_ptr, size));

    lck.unlock();
    this->_ready_cond.notify_one();
}

std::uint8_t rwg_http::buf_instream::getc() {
    if (this->_using_unit_pos >= this->_using_unit_size) {
        this->__flush();
    }

    return this->_using_unit[this->_using_unit_pos++];
}

rwg_http::buf_outstream::buf_outstream(rwg_http::buffer&& buffer,
                                       std::size_t unit_size,
                                       std::function<void (std::uint8_t*, std::size_t n)> sync_func,
                                       std::function<void (rwg_http::buf_outstream&)> notify_func)
    : _buffer(std::move(buffer))
    , _unit_size(unit_size)
    , _using_unit(nullptr)
    , _using_unit_size(0)
    , _sync(sync_func)
    , _notify(notify_func) {
    
    int unit_count = this->_buffer.size() / unit_size;
    for (auto i = 0; i < unit_count; i++) {
        this->_free.push(this->_buffer.get() + (i * unit_size));
    }
}

void rwg_http::buf_outstream::__flush() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_free_cond.wait(lck, [this] () -> bool { return !this->_free.empty(); });

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
}

void rwg_http::buf_outstream::flush() {
    if (this->_using_unit_size == 0) {
        return;
    }
    this->__flush();

    std::unique_lock<std::mutex> lck(this->_mtx);
    if (this->_ready.empty()) {
        return;
    }
    auto unit = this->_ready.front();
    this->_ready.pop();

    this->_sync(unit.first, unit.second);
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();

    lck.lock();
    this->_free_cond.wait(lck, [this] () -> bool { return this->_ready.empty(); });
}

void rwg_http::buf_outstream::sync() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    this->_ready_cond.wait(lck, [this] () -> bool { return !this->_ready.empty(); });
    
    auto unit = this->_ready.front();
    this->_ready.pop();

    this->_sync(unit.first, unit.second);
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();
}

void rwg_http::buf_outstream::nonblock_sync() {
    std::unique_lock<std::mutex> lck(this->_mtx);
    if (this->_ready.empty()) {
        return;
    }

    auto unit = this->_ready.front();
    this->_ready.pop();

    this->_sync(unit.first, unit.second);
    this->_free.push(unit.first);

    lck.unlock();
    this->_free_cond.notify_one();
}

void rwg_http::buf_outstream::putc(std::uint8_t c) {
    if (this->_using_unit == nullptr || this->_using_unit_size == this->_unit_size) {
        this->__flush();
    }

    this->_using_unit[this->_using_unit_size++] = c;
}
