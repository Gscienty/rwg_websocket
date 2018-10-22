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
    std::basic_string<uint8_t> _payload;
    std::uint32_t _masking_key;
    
    std::uint8_t __read_byte() {
        std::uint8_t c;
        ::read(this->_fd, &c, 1);
        return c;
    }

    void __parse() {
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
public:
    frame(int fd) : _fd(fd) {
        this->__parse();
    }
};

}

#endif
