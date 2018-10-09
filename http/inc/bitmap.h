#pragma once

#include <cstdint>
#include <memory>

namespace rwg_http {

class bit {
private:
    std::uint8_t& _ref_byte;
    std::uint8_t _mask_byte;
public:
    bit(std::uint8_t& ref_byte, std::uint8_t mask_byte);

    operator bool() const;
    rwg_http::bit& operator=(const bool&& bit);
};

class bitmap {
private:
    std::unique_ptr<std::uint8_t[]> _bits;
    std::size_t _bits_size;
public:
    bitmap(std::size_t bits_size);
    virtual ~bitmap();

    rwg_http::bit operator[](const std::size_t pos);
    std::size_t size() const;
    std::size_t units_size() const;
    void fill(const std::size_t start_pos, const std::size_t end_pos, const bool bit);
    bool ensure(const std::size_t start_pos, const std::size_t end_pos, const bool bit) const;
};

}
