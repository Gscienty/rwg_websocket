#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <utility>
#include <list>

namespace rwg_http {

class buffer {
private:
    std::size_t const _size;
    std::list<std::pair<std::size_t, std::size_t>>::iterator const _pool_itr;
    std::function<void (std::list<std::pair<std::size_t, std::size_t>>::iterator)> _recover;
    std::uint8_t* const _buffer;

    bool _moved;
public:
    buffer(std::function<void (std::list<std::pair<std::size_t, std::size_t>>::iterator)> func,
           std::size_t size,
           std::uint8_t* buffer,
           std::list<std::pair<std::size_t, std::size_t>>::iterator itr);
    buffer(rwg_http::buffer&& bf);
    buffer(rwg_http::buffer& bf);
    virtual ~buffer();

    std::uint8_t& operator[] (const std::size_t pos);
    std::size_t size() const;
    void recover();

    std::uint8_t* get() const;

    template<typename _T_Iter> std::size_t assign(_T_Iter begin, _T_Iter end) {
        std::size_t pos = 0;
        for (auto itr = begin; itr != end; itr++) {
            if (pos >= this->_size) {
                return pos;
            }
            this->_buffer[pos] = *itr;
            pos++;
        }

        return pos;
    }

    template<typename _T_Input_Iter> std::size_t insert(_T_Input_Iter begin, _T_Input_Iter end, const std::size_t pos) {
        std::size_t ret = 0;
        size_t realpos = pos;
        for (auto itr = begin; itr != end; itr++) {
            if (realpos >= this->_size) {
                break;
            }

            this->_buffer[realpos] = *itr;
            realpos++;
            ret++;
        }
        return ret;
    }

    template<typename _T_Output_Iter> void copy_to(const std::size_t begin_pos, const std::size_t end_pos, _T_Output_Iter output_itr) {
        if (begin_pos >= end_pos) {
            return;
        }

        if (begin_pos >= this->_size || end_pos > this->_size) {
            throw std::out_of_range("rwg_http::buffer_pool::copty_to: out of range");
        }

        _T_Output_Iter itr = output_itr;
        for (auto pos = begin_pos; pos < end_pos; pos++) {
            *itr = this->_buffer[pos];
            itr++;
        }
    }
};

}
