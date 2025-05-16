#include "../include/util.hpp"

#include <iostream>
#include <sstream>

namespace KCP {

    bool isRequireConnect(const char* buffer, int len) {
        return KCP_CONNECT_PACKET.compare(std::string(buffer, len));
    }

    std::string GenerateSendBackConvMsg(uint32_t conv) {
        std::ostringstream ossm;
        ossm << KCP_SEND_CONV_PACKET << conv;
        return ossm.str();
    }

    std::string GenerateDisconnectMsg(uint32_t conv) {
        std::ostringstream ossm;
        ossm << KCP_DISCONNECT_PACKET << conv;
        return ossm.str();
    }
};