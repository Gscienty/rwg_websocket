#pragma once

#include "buffer_pool.h"
#include <streambuf>
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <condition_variable>

namespace rwg_http {

class buf_stream {
private:
    rwg_http::buffer& _buffer;
    std::size_t _bpos;
    std::size_t _pos;
    std::size_t _epos;

    std::mutex _use_mtx;
    std::condition_variable _useable_cond;
    std::condition_variable _unuseable_cond;

    void bump(std::size_t size);
    void wait_useable(std::unique_lock<std::mutex>& locker);
    void wait_unuseable(std::unique_lock<std::mutex>& locker);
public:
    buf_stream(rwg_http::buffer& buffer);

    std::size_t bpos() const;
    std::size_t pos() const;
    std::size_t epos() const;

    bool is_end() const;
    void clear();

    std::uint8_t getc();
    void putc(std::uint8_t c);

    template<typename _T_Output_Iter> std::size_t gets(_T_Output_Iter s, std::size_t n) {
        std::unique_lock<std::mutex> locker(this->_use_mtx);
        this->wait_useable(locker);

        auto get_size = std::min(n, this->epos() - this->pos());
        this->_buffer.copy_to(this->_pos, this->_pos + get_size, s);
        this->bump(get_size);
        return get_size;
    }

    template<typename _T_Input_Iter> std::size_t puts(_T_Input_Iter begin, _T_Input_Iter end) {
        std::unique_lock<std::mutex> locker(this->_use_mtx);
        this->wait_useable(locker);

        auto put_size = this->_buffer.insert(begin, end, this->_pos);
        this->bump(put_size);
        return put_size;
    }

    // use for in stream
    template<typename _T_Input_Iter> std::size_t load(_T_Input_Iter begin, _T_Input_Iter end) {
        std::unique_lock<std::mutex> locker(this->_use_mtx);

        std::size_t size;
        if (this->is_end()) {
            size = this->_buffer.assign(begin, end);
            this->_bpos = 0;
            this->_pos = 0;
            this->_epos = size;
        }
        else {
            size = this->_buffer.insert(begin, end, this->_epos);
            this->_epos += size;
        }

        this->_useable_cond.notify_one();
        return size;
    }
};

}
