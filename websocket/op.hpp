#ifndef _RWG_WEBSOCKET_OP_
#define _RWG_WEBSOCKET_OP_

#include <cstdint>

namespace rwg_websocket {

enum op : std::uint8_t {
    op_continue = 0x00,
    op_text = 0x01,
    op_bin = 0x02,
    op_close = 0x08,
    op_ping = 0x09,
    op_pong = 0x0A
};

}

#endif
