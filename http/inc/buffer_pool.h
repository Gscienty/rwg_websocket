#pragma once

#include "buffer.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>
#include <utility>
#include <list>

namespace rwg_http {

class buffer_pool {
private:
    std::unique_ptr<std::uint8_t[]> _pool;
    std::mutex _pool_mtx;

    std::list<std::pair<std::size_t, std::size_t>> _usable;
    std::list<std::pair<std::size_t, std::size_t>> _unusable;

    std::list<std::pair<std::size_t, std::size_t>>::iterator __use(std::pair<std::size_t, std::size_t> block);
    std::size_t __calc_min_demand(std::size_t size) const;
    void __recover(std::list<std::pair<std::size_t, std::size_t>>::iterator func);
public:
    buffer_pool(std::size_t pool_size);

    std::unique_ptr<rwg_http::buffer> alloc(std::size_t demand_size);

    std::list<std::pair<std::size_t, std::size_t>>& usable() { return this->_usable; }
    std::list<std::pair<std::size_t, std::size_t>>& unusable() { return this->_unusable; }
};

}
