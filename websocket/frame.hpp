#ifndef _RWG_WEBSOCKET_FRAME_
#define _RWG_WEBSOCKET_FRAME_

#include "op.hpp"
#include <cstdint>
#include <string>
#include <unistd.h>
#ifdef DEBUG
#include <iostream>
#endif

namespace rwg_websocket {

enum frame_parse_stat {
    fpstat_first_byte,
    fpstat_second_byte,
    fpstat_payloadlen2_1,
    fpstat_payloadlen2_2,
    fpstat_payloadlen8_1,
    fpstat_payloadlen8_2,
    fpstat_payloadlen8_3,
    fpstat_payloadlen8_4,
    fpstat_payloadlen8_5,
    fpstat_payloadlen8_6,
    fpstat_payloadlen8_7,
    fpstat_payloadlen8_8,
    fpstat_mask_1,
    fpstat_mask_2,
    fpstat_mask_3,
    fpstat_mask_4,
    fpstat_payload,
    fpstat_end,
    fpstat_interrupt,
    fpstat_err,
    fpstat_next,
};

class frame {
private:
    int _fd;
    bool _fin_flag;
    rwg_websocket::op _opcode;
    bool _mask;
    std::uint64_t _payload_len;
    std::uint64_t _payload_loaded_count;
    std::basic_string<std::uint8_t> _payload;
    std::basic_string<std::uint8_t> _masking_key;

    rwg_websocket::frame_parse_stat _stat;
    
    std::uint8_t __read_byte() {
        std::uint8_t c;
        ::ssize_t ret = ::read(this->_fd, &c, 1);
        if (ret == 0) {
            this->_stat = rwg_websocket::fpstat_interrupt;
        }
        if (ret == -1) {
            this->_stat = rwg_websocket::fpstat_next;
        }
        return c;
    }

public:
    frame()
        : _fd(0)
        , _fin_flag(false)
        , _mask(false)
        , _payload_len(0)
        , _payload_loaded_count(0)
        , _stat(rwg_websocket::fpstat_first_byte) {
        this->_masking_key.resize(4, 0);
    }

    bool& fin_flag() { return this->_fin_flag; }
    rwg_websocket::op& opcode() { return this->_opcode; }
    bool& mask() { return this->_mask; }
    std::basic_string<std::uint8_t>& payload() { return this->_payload; }
    std::basic_string<std::uint8_t>& masking_key() { return this->_masking_key; }
    int& fd() { return this->_fd; }
    rwg_websocket::frame_parse_stat& stat() { return this->_stat; }

    void reset() {
        this->_fin_flag = false;
        this->_mask = false;
        this->_payload_len = 0;
        this->_payload_loaded_count = 0;
        this->_stat = rwg_websocket::fpstat_first_byte;
        this->_masking_key.clear();
        this->_masking_key.resize(4, 0);
        this->_payload.clear();
    }

    void write() {
        std::uint8_t c = 0x00;
        if (this->_fin_flag) {
            c |= 0x80;
        }
        c |= this->_opcode;
        ::write(this->_fd, &c, 1);

        c = 0x00;
        if (this->_mask) {
            c |= 0x80;
        }

        if (this->_payload.size() < 126) {
            c |= static_cast<std::uint8_t>(this->_payload.size());
            ::write(this->_fd, &c, 1);
        }
        else if (this->_payload.size() <= 0xFFFF) {
            c |= static_cast<std::uint8_t>(126);
            ::write(this->_fd, &c, 1);
            std::uint8_t cs[2];
            cs[0] = (this->_payload.size() & 0xFF00) >> 8;
            cs[1] = (this->_payload.size() & 0x00FF);

            ::write(this->_fd, cs, 2);
        }
        else {
            c |= static_cast<std::uint8_t>(127);
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
            ::write(this->_fd, this->_masking_key.data(), 4);

            for (auto i = 0UL; i < this->_payload.size(); i++) {
                this->_payload[i] ^= this->_masking_key[i % 4];
            }
        }

        ::write(this->_fd, this->_payload.data(), this->_payload.size());
    }

    void parse() {
#ifdef DEBUG
        std::cout << "websocket frame parsing" << std::endl;
#endif
        while (this->_stat != rwg_websocket::fpstat_end &&
               this->_stat != rwg_websocket::fpstat_err &&
               this->_stat != rwg_websocket::fpstat_interrupt &&
               this->_stat != rwg_websocket::fpstat_next) {
            std::uint8_t c = this->__read_byte();
            if (this->_stat == rwg_websocket::fpstat_interrupt ||
                this->_stat == rwg_websocket::fpstat_err ||
                this->_stat == rwg_websocket::fpstat_end ||
                this->_stat == rwg_websocket::fpstat_next) {
#ifdef DEBUG
                std::cout << "websocket farame parsing need next" << std::endl;
#endif
                break;
            }

            switch (this->_stat) {
            default:
                break;
            case rwg_websocket::fpstat_first_byte:
                if ((c & 0x80) != 0) {
                    this->_fin_flag = true;
                }
                else {
                    this->_fin_flag = false;
                }
#ifdef DEBUG
                std::cout << "fin flag: " << this->_fin_flag << std::endl;
#endif
                this->_opcode = static_cast<rwg_websocket::op>(c & 0x0F);
                this->_stat = rwg_websocket::fpstat_second_byte;
                break;
            case rwg_websocket::fpstat_second_byte:
                if ((c & 0x80) != 0) {
                    this->_mask = true;
                }
                else {
                    this->_mask = false;
                }
#ifdef DEBUG
                std::cout << "mask flag:" << this->_mask << std::endl;
#endif
                this->_payload_len = (c & 0x7F);
                if (this->_payload_len == 126) {
                    this->_payload_len = 0;
                    this->_stat = rwg_websocket::fpstat_payloadlen2_1;
                }
                else if (this->_payload_len == 127) {
                    this->_payload_len = 0;
                    this->_stat = rwg_websocket::fpstat_payloadlen8_1;
                }
                else {
                    this->_payload.resize(this->_payload_len);
                    if (this->_mask) {
                        this->_stat = rwg_websocket::fpstat_mask_1;
                    }
                    else {
                        this->_stat = rwg_websocket::fpstat_payload;
                    }
                }
                break;
            case rwg_websocket::fpstat_payloadlen2_1:
                this->_payload_len = static_cast<uint64_t>(c) << 8;
                this->_mask = rwg_websocket::fpstat_payloadlen2_2;
                break;
            case rwg_websocket::fpstat_payloadlen2_2:
                this->_payload_len |= static_cast<uint64_t>(c);
                if (this->_mask) {
                    this->_stat = rwg_websocket::fpstat_mask_1;
                }
                else {
                    this->_stat = rwg_websocket::fpstat_payload;
                }
                break;
            case rwg_websocket::fpstat_payloadlen8_1:
                this->_payload_len = static_cast<uint64_t>(c) << 56;
                this->_stat = rwg_websocket::fpstat_payloadlen8_2;
                break;
            case rwg_websocket::fpstat_payloadlen8_2:
                this->_payload_len |= static_cast<uint64_t>(c) << 48;
                this->_stat = rwg_websocket::fpstat_payloadlen8_3;
                break;
            case rwg_websocket::fpstat_payloadlen8_3:
                this->_payload_len |= static_cast<uint64_t>(c) << 40;
                this->_stat = rwg_websocket::fpstat_payloadlen8_4;
                break;
            case rwg_websocket::fpstat_payloadlen8_4:
                this->_payload_len |= static_cast<uint64_t>(c) << 32;
                this->_stat = rwg_websocket::fpstat_payloadlen8_5;
                break;
            case rwg_websocket::fpstat_payloadlen8_5:
                this->_payload_len |= static_cast<uint64_t>(c) << 24;
                this->_stat = rwg_websocket::fpstat_payloadlen8_6;
                break;
            case rwg_websocket::fpstat_payloadlen8_6:
                this->_payload_len |= static_cast<uint64_t>(c) << 16;
                this->_stat = rwg_websocket::fpstat_payloadlen8_7;
                break;
            case rwg_websocket::fpstat_payloadlen8_7:
                this->_payload_len |= static_cast<uint64_t>(c) << 8;
                this->_stat = rwg_websocket::fpstat_payloadlen8_8;
                break;
            case rwg_websocket::fpstat_payloadlen8_8:
                this->_payload_len |= static_cast<uint64_t>(c);
                if (this->_mask) {
                    this->_stat = rwg_websocket::fpstat_mask_1;
                }
                else {
                    this->_stat = rwg_websocket::fpstat_payload;
                }
                break;
            case rwg_websocket::fpstat_mask_1:
                this->_masking_key[0] = c;
                this->_stat = rwg_websocket::fpstat_mask_2;
                break;
            case rwg_websocket::fpstat_mask_2:
                this->_masking_key[1] = c;
                this->_stat = rwg_websocket::fpstat_mask_3;
                break;
            case rwg_websocket::fpstat_mask_3:
                this->_masking_key[2] = c;
                this->_stat = rwg_websocket::fpstat_mask_4;
                break;
            case rwg_websocket::fpstat_mask_4:
                this->_masking_key[3] = c;
                this->_stat = rwg_websocket::fpstat_payload;
                break;
            case rwg_websocket::fpstat_payload:
                this->_payload[this->_payload_loaded_count] =
                    c ^ this->_masking_key[this->_payload_loaded_count % 4];
                this->_payload_loaded_count++;
                if (this->_payload_len == this->_payload_loaded_count) {
                    this->_stat = rwg_websocket::fpstat_end;
                }
                break;
            }
        }
    }
};

}

#endif
