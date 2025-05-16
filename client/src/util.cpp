#include "util.h"

#include <stdlib.h>

namespace KCP {

    uint32_t getKcpConv(const char* buffer, int len) {
        int head_len = KCP_SEND_CONV_PACKET.length();
        if ((len > head_len) && (0 == KCP_SEND_CONV_PACKET.compare(std::string(buffer, head_len)))) {
            return atol(buffer+head_len);
        }
        return NOT_KCP_CONNECT_PACK;
    }

};