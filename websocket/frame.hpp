#ifndef _RWG_WEBSOCKET_FRAME_
#define _RWG_WEBSOCKET_FRAME_

#include "op.hpp"
#include <cstdint>
#include <string>
#include <unistd.h>

namespace rwg_websocket {

class frame {
private:
    int _fd;
    bool _fin_flag;
    rwg_websocket::op _opcode;
    bool _mask;
    std::basic_string<std::uint8_t> _payload;
    std::uint32_t _masking_key;
    
    std::uint8_t __read_byte() {
        std::uint8_t c;
        ::read(this->_fd, &c, 1);
        return c;
    }

public:
    frame(int fd) : _fd(fd) {
    }

    bool& fin_flag() { return this->_fin_flag; }
    rwg_websocket::op& opcode() { return this->_opcode; }
    bool& mask() { return this->_mask; }
    std::basic_string<std::uint8_t>& payload() { return this->_payload; }
    std::uint32_t& masking_key() { return this->_masking_key; }

    void write() {
        std::uint8_t c = 0x00;
        if (this->_fin_flag) {
            c |= 0x01;
        }
        c |= this->_opcode << 4;
        ::write(this->_fd, &c, 1);

        c = 0x00;
        if (this->_mask) {
            c |= 0x01;
        }

        if (this->_payload.size() < 126) {
            c |= static_cast<std::uint8_t>(this->_payload.size() << 1);
            ::write(this->_fd, &c, 1);
        }
        else if (this->_payload.size() <= 0xFFFF) {
            c = 126;
            ::write(this->_fd, &c, 1);
            std::uint8_t cs[2];
            cs[0] = (this->_payload.size() & 0xFF00) >> 8;
            cs[1] = (this->_payload.size() & 0x00FF);

            ::write(this->_fd, cs, 2);
        }
        else {
            c = 127;
            ::write(this->_fd, &c, 1);
            std::uint8_t cs[8];
            cs[0] = (this->_payload.size() & 0xFF00000000000000) >> 56;
            cs[1] = (this->_payload.size() & 0x00FF000000000000) >> 48;
            cs[2] = (this->_payload.size() & 0x0000FF0000000000) >> 40;
            cs[3] = (this->_payload.size() & 0x000000FF00000000) >> 32;
            cs[4] = (this->_payload.size() & 0x00000000FF000000) >> 24;
            cs[5] = (this->_payload.size() & 0x0000000000FF0000) >> 16;
            cs[6] = (this->_payload.size() & 0x000000000000FF00) >> 8;
            cs[7] = (this->_payload.size() & 0x00000000000000FF);

            ::write(this->_fd, cs, 8);
        }

        if (this->_mask) {
            std::uint8_t cs[4];

            cs[0] = (this->_masking_key & 0xFF000000) >> 24;
            cs[1] = (this->_masking_key & 0x00FF0000) >> 16;
            cs[2] = (this->_masking_key & 0x0000FF00) >> 8;
            cs[3] = (this->_masking_key & 0x000000FF);

            ::write(this->_fd, cs, 4);
        }

        ::write(this->_fd, this->_payload.data(), this->_payload.size());
    }

    void parse() {
#ifdef DEBUG
        std::cout << "websocket frame parsing" << std::endl;
#endif
        std::uint8_t c = this->__read_byte();
        if ((c & 0x01) != 0) {
            this->_fin_flag = true;
        }
        else {
            this->_fin_flag = false;
        }
        this->_opcode = static_cast<rwg_websocket::op>((c & 0xF0) >> 4);

        c = this->__read_byte();
        if ((c & 0x01) != 0) {
            this->_mask = true;
        }
        else {
            this->_mask = false;
        }

        std::uint64_t payload_len = (c & 0xFE) >> 1;
        if (payload_len == 126) {
            std::uint8_t c1 = this->__read_byte();
            std::uint8_t c2 = this->__read_byte();

            payload_len = (static_cast<std::uint64_t>(c1) << 8) | (static_cast<std::uint64_t>(c2));
        }
        else if (payload_len == 127) {
            std::uint8_t cs[8];
            for (auto i = 0; i < 8; i++) {
                cs[i] = this->__read_byte();
            }

            payload_len = (static_cast<std::uint64_t>(cs[0]) << 56) |
                (static_cast<std::uint64_t>(cs[1]) << 48) |
                (static_cast<std::uint64_t>(cs[2]) << 40) |
                (static_cast<std::uint64_t>(cs[3]) << 32) |
                (static_cast<std::uint64_t>(cs[4]) << 24) |
                (static_cast<std::uint64_t>(cs[5]) << 16) |
                (static_cast<std::uint64_t>(cs[6]) << 8) |
                (static_cast<std::uint64_t>(cs[7]));
        }

        if (this->_mask) {
            std::uint8_t cs[4];
            for (auto i = 0; i < 4; i++) {
                cs[i] = this->__read_byte();
            }

            this->_masking_key = (static_cast<std::uint32_t>(cs[0]) << 24) |
                (static_cast<std::uint32_t>(cs[1]) << 16) |
                (static_cast<std::uint32_t>(cs[2]) << 8) |
                (static_cast<std::uint32_t>(cs[3]));
        }

        this->_payload.resize(payload_len);
        ::read(this->_fd, const_cast<std::uint8_t *>(this->_payload.data()), payload_len);
    }
};

}

#endif
