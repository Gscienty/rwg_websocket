#include "buf_stream.h"

rwg_http::buf_stream::buf_stream(rwg_http::buffer& buffer)
    : _buffer(buffer)
    , _bpos(0)
    , _pos(0)
    , _epos(buffer.size()) {

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
    this->_buffer.fill(this->_bpos, this->_epos, 0x00);
}

std::uint8_t& rwg_http::buf_stream::getc() const {
    return this->_buffer[this->_pos];
}

void rwg_http::buf_stream::bump(std::size_t size) {
    this->_pos = std::min(this->_pos + size, this->_epos);
}

void rwg_http::buf_stream::putc(std::uint8_t c) {
    if (this->is_end()) {
        return;
    }
    this->_buffer[this->_pos++] = c;
}
