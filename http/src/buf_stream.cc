#include "buf_stream.h"

rwg_http::buf_stream::buf_stream(rwg_http::buffer& buffer)
    : _buffer(buffer)
    , _bpos(0)
    , _pos(0)
    , _epos(0) {

    this->_buffer.fill(0, this->_epos, 0x00);
}

std::size_t rwg_http::buf_stream::bpos() const {
    return this->_bpos;
}

std::size_t rwg_http::buf_stream::pos() const {
    return this->_pos;
}

std::size_t rwg_http::buf_stream::epos() const {
    return this->_epos;
}

bool rwg_http::buf_stream::is_end() const {
    return this->_pos == this->_epos;
}

void rwg_http::buf_stream::clear() {
    std::unique_lock<std::mutex> locker(this->_use_mtx);
    this->wait_unuseable(locker);

    this->_bpos = 0;
    this->_pos = 0;
    this->_epos = this->_buffer.size();
    this->_useable_cond.notify_one();
}

std::uint8_t rwg_http::buf_stream::getc() {
    std::unique_lock<std::mutex> locker(this->_use_mtx);
    this->wait_useable(locker);

    std::uint8_t c = this->_buffer[this->_pos];
    this->bump(1);
    return c;
}

void rwg_http::buf_stream::bump(std::size_t size) {
    this->_pos = std::min(this->_pos + size, this->_epos);
    this->_unuseable_cond.notify_one();
}

void rwg_http::buf_stream::wait_useable(std::unique_lock<std::mutex>& locker) {
    this->_useable_cond.wait(locker, [this] () -> bool { return !this->is_end(); });
}

void rwg_http::buf_stream::wait_unuseable(std::unique_lock<std::mutex>& locker) {
    this->_unuseable_cond.wait(locker, [this] () -> bool { return this->is_end(); });

}

void rwg_http::buf_stream::putc(std::uint8_t c) {
    std::unique_lock<std::mutex> locker(this->_use_mtx);
    this->wait_useable(locker);

    this->_buffer[this->_pos] = c;
    this->bump(1);
}

