#include "websocket/frame.hpp"
#include "util/debug.hpp"
#include <openssl/ssl.h>
#include <sstream>

namespace rwg_websocket {
frame::frame()
    : _fd(0)
    , _security(false)
    , _ssl(nullptr)
    , _fin_flag(false)
    , _mask(false)
    , _payload_len(0)
    , _payload_loaded_count(0)
    , _stat(rwg_websocket::fpstat_first_byte) {
    this->_masking_key.resize(4, 0);
}

std::uint8_t frame::__read_byte() {
    std::uint8_t c;
    if (this->_security) {
        if (this->_ssl == nullptr) {
            error("ws_frame[%d]: ssl is NULL", this->_fd);
            return 0;
        }
        ::ssize_t ret = ::SSL_read(this->_ssl, &c, 1);
        if (ret <= 0) {
            if (ret == 0) {
                warn("ws_frame[%d]: tls read error (part 1)", this->_fd);
                this->_stat = rwg_websocket::fpstat_interrupt;
            }
            else if (SSL_get_error(this->_ssl, ret) != SSL_ERROR_WANT_READ) {
                warn("ws_frame[%d]: tls read error (part 2)", this->_fd);
                this->_stat = rwg_websocket::fpstat_interrupt;
            }
            else {
                info("ws_frame[%d]: tls need read next", this->_fd);
                this->_stat = rwg_websocket::fpstat_next;
            }
        }
    }
    else {
        ::ssize_t ret = ::read(this->_fd, &c, 1);
        if (ret == 0) {
            this->_stat = rwg_websocket::fpstat_interrupt;
        }
        if (ret == -1) {
            this->_stat = rwg_websocket::fpstat_next;
        }

    }
    return c;
}

bool& frame::fin_flag() { return this->_fin_flag; }

rwg_websocket::op &frame::opcode() { return this->_opcode; }

std::basic_string<std::uint8_t> &frame::payload() { return this->_payload; }

std::basic_string<std::uint8_t> &frame::masking_key() { return this->_masking_key; }

int &frame::fd() { return this->_fd; }

SSL *&frame::ssl() { return this->_ssl; }

bool &frame::security() { return this->_security; }

rwg_websocket::frame_parse_stat &frame::stat() { return this->_stat; }

void frame::reset() {
    this->_fin_flag = false;
    this->_mask = false;
    this->_payload_len = 0;
    this->_payload_loaded_count = 0;
    this->_stat = rwg_websocket::fpstat_first_byte;
    this->_masking_key.clear();
    this->_masking_key.resize(4, 0);
    this->_payload.clear();
}

void frame::write() {
    std::basic_stringstream<uint8_t> sstr;

    std::uint8_t c = 0x00;
    if (this->_fin_flag) {
        c |= 0x80;
    }
    c |= this->_opcode;
    /* ::write(this->_fd, &c, 1); */
    sstr.put(c);

    c = 0x00;
    if (this->_mask) {
        c |= 0x80;
    }

    if (this->_payload.size() < 126) {
        c |= static_cast<std::uint8_t>(this->_payload.size());
        /* ::write(this->_fd, &c, 1); */
        sstr.put(c);
    }
    else if (this->_payload.size() <= 0xFFFF) {
        c |= static_cast<std::uint8_t>(126);
        /* ::write(this->_fd, &c, 1); */
        sstr.put(c);

        std::uint8_t cs[2];
        cs[0] = (this->_payload.size() & 0xFF00) >> 8;
        cs[1] = (this->_payload.size() & 0x00FF);

        /* ::write(this->_fd, cs, 2); */
        sstr.write(cs, 2);
    }
    else {
        c |= static_cast<std::uint8_t>(127);
        /* ::write(this->_fd, &c, 1); */
        sstr.put(c);

        std::uint8_t cs[8];
        cs[0] = (this->_payload.size() & 0xFF00000000000000) >> 56;
        cs[1] = (this->_payload.size() & 0x00FF000000000000) >> 48;
        cs[2] = (this->_payload.size() & 0x0000FF0000000000) >> 40;
        cs[3] = (this->_payload.size() & 0x000000FF00000000) >> 32;
        cs[4] = (this->_payload.size() & 0x00000000FF000000) >> 24;
        cs[5] = (this->_payload.size() & 0x0000000000FF0000) >> 16;
        cs[6] = (this->_payload.size() & 0x000000000000FF00) >> 8;
        cs[7] = (this->_payload.size() & 0x00000000000000FF);

        /* ::write(this->_fd, cs, 8); */
        sstr.write(cs, 8);
    }

    if (this->_mask) {
        /* ::write(this->_fd, this->_masking_key.data(), 4); */
        sstr.write(this->_masking_key.data(), 4);

        for (auto i = 0UL; i < this->_payload.size(); i++) {
            this->_payload[i] ^= this->_masking_key[i % 4];
        }
    }

    /* ::write(this->_fd, this->_payload.data(), this->_payload.size()); */
    sstr.write(this->_payload.data(), this->_payload.size());

    if (this->_security) {
        ::SSL_write(this->_ssl, sstr.str().data(), sstr.str().size());
    }
    else {
        ::write(this->_fd, sstr.str().data(), sstr.str().size());
    }
}

void frame::parse() {
    info("ws_frame[%d]: parsing", this->_fd);
    while (this->_stat != rwg_websocket::fpstat_end &&
           this->_stat != rwg_websocket::fpstat_err &&
           this->_stat != rwg_websocket::fpstat_interrupt &&
           this->_stat != rwg_websocket::fpstat_next) {
        std::uint8_t c = this->__read_byte();
        if (this->_stat == rwg_websocket::fpstat_interrupt ||
            this->_stat == rwg_websocket::fpstat_err ||
            this->_stat == rwg_websocket::fpstat_end ||
            this->_stat == rwg_websocket::fpstat_next) {
            info("ws_frame[%d]: parsing need next", this->_fd);
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
            info("ws_frame[%d]: fin flag [%d]", this->_fd, this->_fin_flag);
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
            info("ws_frame[%d]: mask flag: [%d]", this->_fd, this->_mask);
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
                info("ws_frame[%d]: payload length [%lu]", this->_fd, this->_payload_len);
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
            info("ws_frame[%d]: payload length [%lu]", this->_fd, this->_payload_len);
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
            info("ws_frame[%d]: payload length [%lu]", this->_fd, this->_payload_len);
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
            info("ws_frame[%d]: masking key %2x %2x %2x %2x",
                 this->_fd, this->_masking_key[0], this->_masking_key[1], this->_masking_key[2], this->_masking_key[3]);
            this->_stat = rwg_websocket::fpstat_payload;
            break;
        case rwg_websocket::fpstat_payload:
            this->_payload[this->_payload_loaded_count] =
                c ^ this->_masking_key[this->_payload_loaded_count % 4];
            this->_payload_loaded_count++;
            if (this->_payload_len == this->_payload_loaded_count) {
                info("ws_frame[%d]: parse completed", this->_fd);
                this->_stat = rwg_websocket::fpstat_end;
            }
            break;
        }
    }
}

}
