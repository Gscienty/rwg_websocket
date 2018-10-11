#pragma once

#include "bitmap.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <utility>

namespace rwg_http {

class buffer_pool;

class buffer {
private:
    std::size_t _unit_size;
    std::size_t _size;
    std::size_t _unit_off;
    std::function<void (std::function<void (std::function<void (std::size_t pos)>)> func)> _recover;
    std::vector<std::pair<std::size_t, std::uint8_t*>> _units;

    bool _moved;
public:
    buffer(std::function<void (std::function<void (std::function<void (std::size_t pos)>)> func)> recover,
           std::size_t unit_size,
           std::size_t size,
           std::vector<std::pair<std::size_t, std::uint8_t*>>&& units);
    buffer(rwg_http::buffer&& bf);
    virtual ~buffer();

    std::uint8_t& operator[] (const std::size_t pos);
    std::size_t size() const;
    std::size_t avail_size() const;
    std::size_t unit_size() const;
    void recover();
    void fill(const std::size_t begin_pos, const std::size_t end_pos, const std::uint8_t val);

    std::uint8_t* unit(const std::size_t n) const;
    std::size_t unit_index(const std::size_t pos) const;

    void head_move_tail();

    rwg_http::buffer split(const std::size_t size);

    template<typename _T_Iter> std::size_t assign(_T_Iter begin, _T_Iter end) {
        std::size_t pos = 0;
        for (auto itr = begin; itr != end; itr++) {
            if (pos >= this->_size) {
                return pos;
            }
            this->_units[(this->_unit_off + pos / this->_unit_size) % this->_units.size()]
                .second[pos % this->_unit_size] = *itr;
            pos++;
        }

        return pos;
    }

    template<typename _T_Input_Iter> std::size_t insert(_T_Input_Iter begin, _T_Input_Iter end, const std::size_t pos) {
        std::size_t ret = 0;
        size_t realpos = pos;
        for (auto iter = begin; iter != end; iter++) {
            if (realpos >= this->_size) {
                break;
            }

            this->_units[(this->_unit_off + realpos / this->_unit_size) % this->_units.size()]
                .second[realpos % this->_unit_size] = *iter;
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
            *itr = this->_units[(this->_unit_off + pos / this->_unit_size) % this->_units.size()]
                .second[pos % this->_unit_size];
            itr++;
        }
    }
};

class buffer_pool {
private:
    std::unique_ptr<std::uint8_t[]> _pool;
    std::size_t _unit_size;
    std::size_t _unit_count;
    rwg_http::bitmap _map;
    std::mutex _pool_mtx;

    void __recover(std::function<void (std::function<void (std::size_t pos)>)> func);
public:
    buffer_pool(std::size_t unit_size, std::size_t unit_count);

    std::size_t unit_size() const;
    std::size_t unit_count() const;
    rwg_http::buffer alloc(std::size_t demand_size);
    const rwg_http::bitmap& map();
};

}
