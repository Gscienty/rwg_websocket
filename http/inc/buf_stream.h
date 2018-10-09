#pragma once

#include "buffer_pool.h"
#include <streambuf>
#include <cstdint>
#include <algorithm>

namespace rwg_http {

class buf_stream {
private:
    rwg_http::buffer& _buffer;
    std::size_t _bpos;
    std::size_t _pos;
    std::size_t _epos;

public:
    buf_stream(rwg_http::buffer& buffer);

    std::size_t bpos() const;
    std::size_t pos() const;
    std::size_t epos() const;

    bool is_end() const;
    void clear();

    std::uint8_t& getc() const;
    void bump(std::size_t size);
    void putc(std::uint8_t c);

    template<typename _T_Output_Iter> std::size_t gets(_T_Output_Iter s, std::size_t n) {
        auto get_size = std::min(n, this->epos() - this->pos());
        this->_buffer.copy_to(this->_pos, this->_pos + get_size, s);
        this->bump(get_size);
        return get_size;
    }

    template<typename _T_Input_Iter> std::size_t puts(_T_Input_Iter begin, _T_Input_Iter end) {
        auto put_size = this->_buffer.insert(begin, end, this->_pos);
        this->bump(put_size);
        return put_size;
    }
};

}
