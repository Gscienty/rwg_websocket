#include "buffer_pool.h"
#include <stdexcept>
#include <algorithm>

rwg_http::buffer_pool::buffer_pool(std::size_t unit_size, std::size_t unit_count)
    : _pool(new std::uint8_t[unit_size * unit_count])
    , _unit_size(unit_size)
    , _unit_count(unit_count)
    , _map(unit_count) {}

std::size_t rwg_http::buffer_pool::unit_count() const {
    return this->_unit_count;
}

std::size_t rwg_http::buffer_pool::unit_size() const {
    return this->_unit_size;
}

rwg_http::buffer rwg_http::buffer_pool::alloc(std::size_t demand_size) {
    std::size_t unit_count = demand_size / this->_unit_size + 
        (demand_size % this->_unit_size == 0 ? 0 : 1);

    std::lock_guard<std::mutex> locker(this->_pool_mtx);

    std::size_t avail_unit_count = 0;
    for (std::size_t i = 0; i < this->_unit_count; i++) {
        if (this->_map[i] == false) {
            avail_unit_count++;
            if (avail_unit_count == unit_count) {
                break;
            }
        }
    }

    if (avail_unit_count != unit_count) {
        throw std::bad_alloc();
    }

    std::vector<std::pair<std::size_t, std::uint8_t*>> units;
    for (std::size_t i = 0; i < this->_unit_count; i++) {
        if (this->_map[i] == false) {
            this->_map[i] = true;
            units.push_back(std::make_pair(i, &this->_pool[i * this->_unit_size]));
            if (--avail_unit_count == 0) {
                break;
            }
        }
    }

    return rwg_http::buffer(std::bind(&rwg_http::buffer_pool::__recover, this, std::placeholders::_1),
                            this->_unit_size,
                            demand_size,
                            units);
}

void rwg_http::buffer_pool::__recover(std::function<void (std::function<void (std::size_t pos)>)> func) {
    std::lock_guard<std::mutex> locker(this->_pool_mtx);
    auto release_unit_func = [this] (std::size_t pos) -> void {
        this->_map[pos] = false;
    };

    func(release_unit_func);
}

const rwg_http::bitmap& rwg_http::buffer_pool::map() {
    return this->_map;
}

rwg_http::buffer::buffer(std::function<void (std::function<void (std::function<void (std::size_t pos)>)> func)> recover,
                         std::size_t unit_size,
                         std::size_t size,
                         std::vector<std::pair<std::size_t, std::uint8_t*>> units)
    : _unit_size(unit_size)
    , _size(size)
    , _recover(recover)
    , _units(units) {}

rwg_http::buffer::~buffer() {
    this->recover();
}

void rwg_http::buffer::recover() {
    auto release_units = [this] (std::function<void (std::size_t pos)> func) -> void {
        for (auto kv : this->_units) {
            func(kv.first);
        }
    };

    this->_recover(release_units);

    this->_units.clear();
}

std::uint8_t& rwg_http::buffer::operator[] (const std::size_t pos) {
    if (pos >= this->_size) {
        throw std::out_of_range("rwg_http::buffer::operator[]: out of range");
    }
    std::size_t unit_pos = pos / this->_unit_size;

    return this->_units[unit_pos].second[pos % this->_unit_size];
}

std::size_t rwg_http::buffer::size() const {
    return this->_size;
}

std::size_t rwg_http::buffer::avail_size() const {
    return this->_units.size() * this->_unit_size;
}

