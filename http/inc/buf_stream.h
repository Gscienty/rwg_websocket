#pragma once

#include "buffer_pool.h"
#include <streambuf>
#include <cstdint>
#include <algorithm>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace rwg_http {

class buf_stream {
private:
    rwg_http::buffer _buffer;
    std::size_t _bpos;
    std::size_t _epos;

    std::mutex _use_mtx;
    std::condition_variable _readable_cond;
    std::condition_variable _writable_cond;

    std::function<void ()> _readable_event;
    std::function<void ()> _writable_event;

    void wait_readable(std::unique_lock<std::mutex>& locker);
    void wait_writable(std::unique_lock<std::mutex>& locker);

    void notify_readable();
    void notify_writable();
public:
    buf_stream(rwg_http::buffer&& buffer);

    std::size_t bpos() const;
    std::size_t epos() const;

    void set_readable_event(std::function<void ()> func);
    void set_writable_event(std::function<void ()> func);

    void clear();

    std::uint8_t getc();
    void putc(std::uint8_t c);

    // use for in stream
    template<typename _T_Input_Iter> void write(_T_Input_Iter s_itr, std::size_t n) {
        std::size_t remain = n;

        _T_Input_Iter itr = s_itr;
        while (remain != 0) {
            std::size_t write_size = std::min(remain, this->_buffer.size() - this->_epos);

            std::unique_lock<std::mutex> locker(this->_use_mtx);
            this->wait_writable(locker);

            this->_buffer.insert(itr, itr + write_size, this->_epos);
            this->_epos += write_size;

            locker.unlock();
            this->notify_readable();

            remain -= write_size;
            itr += write_size;
        }
    }

    // use for out stream
    template<typename _T_Output_Iter> std::size_t read(_T_Output_Iter itr, std::size_t n) {
        std::size_t remain = n;

        if (remain != 0) {
            std::size_t read_size = std::min(remain, this->_epos - this->_bpos);

            std::unique_lock<std::mutex> locker(this->_use_mtx);
            this->wait_readable(locker);

            this->_buffer.copy_to(this->_bpos, this->_bpos + read_size, itr);
            this->_bpos += read_size;

            std::size_t nth_unit = this->_buffer.unit_index(this->_bpos);

            if (nth_unit != 0) {
                for (std::size_t i = 0; i < nth_unit; i++) {
                    this->_buffer.head_move_tail();
                }

                std::size_t off_size = nth_unit * this->_buffer.unit_size();

                this->_bpos -= off_size;
                this->_epos -= off_size;

                this->notify_writable();
            }

            remain -= read_size;
        }

        return n - remain;
    }
};

}
