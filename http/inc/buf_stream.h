#pragma once

#include "buffer_pool.h"
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <atomic>

namespace rwg_http {

class buf_instream {
private:
    rwg_http::buffer _buffer;

    std::size_t _unit_size;

    std::uint8_t* _using_unit;
    std::size_t _using_unit_pos;
    std::size_t _using_unit_size;

    std::queue<std::uint8_t*> _free; 
    std::queue<std::pair<std::uint8_t*, std::size_t>> _ready;

    std::mutex _mtx;
    std::condition_variable _free_cond;
    std::condition_variable _ready_cond;

    std::function<std::size_t (std::uint8_t* s, std::size_t n)> _sync;

    std::atomic<bool> _closed_flag;
    std::function<void ()> _close_callback;

    bool __flush();
public:
    buf_instream(rwg_http::buffer&& buffer,
                 std::size_t unit_size,
                 std::function<std::size_t (std::uint8_t* s, std::size_t n)> flush_func,
                 std::function<void ()> close_callback);

    void sync();
    void close();

    std::uint8_t getc();

    template<typename _T_Output_Itr> void read(_T_Output_Itr s, std::size_t n) {
        std::size_t remain = n;        
        _T_Output_Itr s_pos = s;

        while (remain != 0) {
            if (this->_closed_flag) {
                return;
            }
            if (this->_using_unit_pos == this->_using_unit_size) {
                if (!this->__flush()) {
                    return;
                }
            }

            std::size_t size = std::min(remain, this->_using_unit_size - this->_using_unit_pos);
            std::copy(this->_using_unit + this->_using_unit_pos,
                      this->_using_unit + this->_using_unit_pos + size,
                      s_pos);
            s_pos += size;
            remain -= size;
            this->_using_unit_pos += size;
        }
    }
};

class buf_outstream {
private:
    rwg_http::buffer _buffer;

    std::size_t _unit_size;
    std::uint8_t* _using_unit;
    std::size_t _using_unit_size;
    
    std::queue<std::uint8_t*> _free;
    std::queue<std::pair<std::uint8_t*, std::size_t>> _ready;

    std::mutex _mtx;
    std::condition_variable _free_cond;
    std::condition_variable _ready_cond;

    std::function<bool (std::uint8_t* s, std::size_t n)> _sync;
    std::function<void (rwg_http::buf_outstream&)> _notify;

    std::atomic<bool> _closed_flag;
    std::function<void ()> _close_callback;

    bool __flush();
public:
    buf_outstream(rwg_http::buffer&& buffer,
                  std::size_t unit_size,
                  std::function<bool (std::uint8_t* s, std::size_t n)> sync_func,
                  std::function<void (rwg_http::buf_outstream&)> notify_func,
                  std::function<void ()> close_callback);

    void sync();
    void close();
    void nonblock_sync();
    bool flush();

    void putc(std::uint8_t c);

    template<typename _T_Input_Itr> void write(_T_Input_Itr s, std::size_t n) {
        if (this->_closed_flag) {
            return;
        }
        std::size_t remain = n;
        _T_Input_Itr s_pos = s;

        while (remain != 0) {
            if (this->_using_unit == nullptr || this->_using_unit_size == this->_unit_size) {
                if (!this->__flush()) {
                    return;
                }
            }
            std::size_t size = std::min(remain, this->_unit_size - this->_using_unit_size);

            std::copy(s_pos, s_pos + size, this->_using_unit + this->_using_unit_size);

            s_pos += size;
            remain -= size;
            this->_using_unit_size += size;

            if (this->_using_unit_size == this->_unit_size) {
                if (!this->__flush()) {
                    return;
                }
            }
        }
    }
};

}
