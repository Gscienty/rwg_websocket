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
    const std::size_t _unit_size;
    const std::size_t _size;
    std::function<void (std::function<void (std::function<void (std::size_t pos)>)> func)> _recover;
    std::vector<std::pair<std::size_t, std::uint8_t*>> _units;
public:
    buffer(std::function<void (std::function<void (std::function<void (std::size_t pos)>)> func)> recover,
           std::size_t unit_size,
           std::size_t size,
           std::vector<std::pair<std::size_t, std::uint8_t*>> units);
    virtual ~buffer();

    std::uint8_t& operator[] (const std::size_t pos);
    std::size_t size() const;
    std::size_t avail_size() const;
    void recover();
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
    rwg_http::buffer alloc(size_t demand_size);
    const rwg_http::bitmap& map();
};

}
