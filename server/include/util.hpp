#pragma once

#include <string>
#include <memory>
#include <netinet/in.h>

struct IKCPCB;
typedef struct IKCPCB ikcpcb;
typedef unsigned int uint32_t;

// kcp 发送最低时间间隔
const int KCP_UPDATE_INTERVAL{5}; // ms
const int MAX_MSG_SIZE{1024 * 10}; // 10KB
const uint32_t IKCP_OVERHEAD{24};
const uint32_t KCP_CONNECTION_TIMEOUT_DEADLINE{1000*60}; //10s

const std::string KCP_CONNECT_PACKET("kcp_connection_packet");
const uint32_t NOT_KCP_CONNECT_PACK{2^32-1};
const std::string KCP_SEND_CONV_PACKET("kcp_connection_back_packet conv:");
const std::string KCP_DISCONNECT_PACKET("kcp_disconnect_packet");


namespace KCP {
    // 回调事件类型
    enum eEventType {
        eConnect,           // kcp连接成功回调
        eDisconnect,            // 服务端关闭或异常
        eRecvMsg                // kcp收到消息后解析后raw msg回调
    };
    const char* eventTypeStr(eEventType event_type);

    /** 消息回调函数
     * @param uint32_t   kcp客户端的唯一标识conv
     * @param eEventType 回调事件的消息类型
     * @param string     回调事件的消息体
     */
    typedef void(event_callback_t)(uint32_t, eEventType, std::shared_ptr<std::string>);
    
    bool isRequireConnect(const char* buffer, int len);
    std::string GenerateSendBackConvMsg(uint32_t conv);
    std::string GenerateDisconnectMsg(uint32_t conv);
};

#define KCP_ERR_NOT_EXIST_CONNECTION -1000

