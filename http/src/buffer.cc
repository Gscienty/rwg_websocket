#include "buffer.h"
#include <iostream>

rwg_http::buffer::buffer(std::function<void (std::list<std::pair<std::size_t, std::size_t>>::iterator)> recover,
                         std::size_t size,
                         std::uint8_t* buffer,
                         std::list<std::pair<std::size_t, std::size_t>>::iterator itr)
    : _size(size)
    , _pool_itr(itr)
    , _recover(recover)
    , _buffer(buffer)
    , _moved(false) {}

rwg_http::buffer::buffer(rwg_http::buffer&& buf)
    : _size(buf._size)
    , _pool_itr(buf._pool_itr)
    , _recover(buf._recover)
    , _buffer(buf._buffer)
    , _moved(false) {

    buf._moved = true;
}

rwg_http::buffer::buffer(rwg_http::buffer& buf)
    : _size(buf._size)
    , _pool_itr(buf._pool_itr)
    , _recover(buf._recover)
    , _buffer(buf._buffer)
    , _moved(false) {

    buf._moved = true;
}

rwg_http::buffer::~buffer() {
    this->recover();
}

void rwg_http::buffer::recover() {
    if (this->_moved == false) {
        this->_moved = true;
        this->_recover(this->_pool_itr);
    }
}

std::uint8_t& rwg_http::buffer::operator[] (const std::size_t pos) {
    if (pos >= this->_size) {
        throw std::out_of_range("rwg_http::buffer::operator[]: out of range");
    }

    return this->_buffer[pos];
}

std::size_t rwg_http::buffer::size() const {
    return this->_size;
}


std::uint8_t* rwg_http::buffer::get() const {
    return this->_buffer;
}
