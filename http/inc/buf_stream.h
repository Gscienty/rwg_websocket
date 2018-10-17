#pragma once

#include "buffer_pool.h"
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <atomic>
#include <memory>

#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_http {

class buf_instream {
private:
    std::unique_ptr<rwg_http::buffer> _buffer;

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

    int _log_fd;

    bool __flush();
public:
    buf_instream(std::size_t unit_size,
                 std::function<std::size_t (std::uint8_t* s, std::size_t n)> flush_func,
                 std::function<void ()> close_callback,
                 int fd);

    void sync();
    void close();
    void set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer);
    void release();

    std::uint8_t getc();

    template<typename _T_Output_Itr> void read(_T_Output_Itr s, std::size_t n) {
#ifdef DEBUG
        std::cout << this->_log_fd << " buf_instream: read begin" << std::endl;
#endif
        std::size_t remain = n;        
        _T_Output_Itr s_pos = s;

        while (remain != 0) {
            if (this->_closed_flag) {
#ifdef DEBUG
                std::cout << this->_log_fd << " buf_instream: read interrupt (closed)" << std::endl;
#endif
                return;
            }
            if (this->_using_unit_pos == this->_using_unit_size) {
                if (!this->__flush()) {
#ifdef DEBUG
                    std::cout << this->_log_fd << " buf_instream: read interrupt" << std::endl;
#endif
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
#ifdef DEBUG
        std::cout << this->_log_fd << " bug_instream: read end" << std::endl;
#endif
    }
};

class buf_outstream {
private:
    std::unique_ptr<rwg_http::buffer> _buffer;

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
    int _log_fd;

    bool __flush();
public:
    buf_outstream(std::size_t unit_size,
                  std::function<bool (std::uint8_t* s, std::size_t n)> sync_func,
                  std::function<void (rwg_http::buf_outstream&)> notify_func,
                  std::function<void ()> close_callback,
                  int fd);

    void sync();
    void close();
    void nonblock_sync();
    bool flush();
    void set_buffer(std::unique_ptr<rwg_http::buffer>&& buffer);
    void release();

    void putc(std::uint8_t c);

    template<typename _T_Input_Itr> void write(_T_Input_Itr s, std::size_t n) {
#ifdef DEBUG
        std::cout << "buf_outstream: write begin" << std::endl;
#endif
        if (this->_closed_flag) {
#ifdef DEBUG
            std::cout << "buf_outstream: write interrupt (closed)" << std::endl;
#endif
            return;
        }
        std::size_t remain = n;
        _T_Input_Itr s_pos = s;

        while (remain != 0) {
            if (this->_using_unit == nullptr || this->_using_unit_size == this->_unit_size) {
                if (!this->__flush()) {
#ifdef DEBUG
                    std::cout << "buf_outstream: write interrupt" << std::endl;
#endif
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
#ifdef DEBUG
                    std::cout << "buf_outstream: write interrupt" << std::endl;
#endif
                    return;
                }
            }
        }
#ifdef DEBUG
        std::cout << "buf_outstream: write end" << std::endl;
#endif
    }
};

}
