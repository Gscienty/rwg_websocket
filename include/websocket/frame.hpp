#ifndef _RWG_WEBSOCKET_FRAME_
#define _RWG_WEBSOCKET_FRAME_

#include "op.hpp"
#include <cstdint>
#include <string>
#include <unistd.h>
#include <openssl/ssl.h>

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
    bool _security;
    SSL *_ssl;
    bool _fin_flag;
    rwg_websocket::op _opcode;
    bool _mask;
    std::uint64_t _payload_len;
    std::uint64_t _payload_loaded_count;
    std::basic_string<std::uint8_t> _payload;
    std::basic_string<std::uint8_t> _masking_key;

    rwg_websocket::frame_parse_stat _stat;
    
    std::uint8_t __read_byte();
public:
    frame();

    bool& fin_flag();
    rwg_websocket::op& opcode();
    bool& mask() { return this->_mask; }
    std::basic_string<std::uint8_t> &payload();
    std::basic_string<std::uint8_t> &masking_key();
    int &fd();
    SSL *&ssl();
    void use_security(bool use = true);
    rwg_websocket::frame_parse_stat &stat();
    void reset();
    void write();
    void parse();
};

}

#endif
