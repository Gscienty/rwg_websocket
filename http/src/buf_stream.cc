#include "buf_stream.h"

rwg_http::buf_stream::buf_stream(rwg_http::buffer& buffer)
    : _buffer(buffer)
    , _bpos(0)
    , _epos(0) {

    this->_buffer.fill(0, this->_epos, 0x00);
}

std::size_t rwg_http::buf_stream::bpos() const {
    return this->_bpos;
}

std::size_t rwg_http::buf_stream::epos() const {
    return this->_epos;
}

void rwg_http::buf_stream::clear() {
    std::unique_lock<std::mutex> locker(this->_use_mtx);

    this->_bpos = 0;
    this->_epos = 0;
}

std::uint8_t rwg_http::buf_stream::getc() {
    std::unique_lock<std::mutex> locker(this->_use_mtx);

    this->wait_readable(locker);

    std::uint8_t c = this->_buffer[this->_bpos];
    this->_bpos++;

    if (this->_buffer.unit_index(this->_bpos) != 0) {
        this->_buffer.head_move_tail();

        this->_bpos -= this->_buffer.unit_size();
        this->_epos -= this->_buffer.unit_size();
        
        locker.unlock();
        this->notify_writable();
    }

    return c;
}

void rwg_http::buf_stream::wait_readable(std::unique_lock<std::mutex>& locker) {
    this->_readable_cond.wait(locker, [this] () -> bool { return this->_bpos != this->_epos; });
}

void rwg_http::buf_stream::wait_writable(std::unique_lock<std::mutex>& locker) {
    this->_writable_cond.wait(locker, [this] () -> bool { return this->_epos != this->_buffer.size(); });
}

void rwg_http::buf_stream::notify_readable() {
    this->_readable_cond.notify_one();

    if (this->_readable_event) {
        this->_readable_event();
    }
}

void rwg_http::buf_stream::notify_writable() {
    this->_writable_cond.notify_one();

    if (this->_writable_event) {
        this->_writable_event();
    }
}

void rwg_http::buf_stream::set_readable_event(std::function<void ()> func) {
    this->_readable_event = func;
}

void rwg_http::buf_stream::set_writable_event(std::function<void ()> func) {
    this->_writable_event = func;
}

void rwg_http::buf_stream::putc(std::uint8_t c) {
    std::unique_lock<std::mutex> locker(this->_use_mtx);
    this->wait_writable(locker);


    this->_buffer[this->_epos] = c;
    this->_epos++;

    locker.unlock();
    this->notify_readable();
}

