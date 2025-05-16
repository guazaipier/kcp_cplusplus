#pragma once

#include <string>

struct IKCPCB;
typedef struct IKCPCB ikcpcb;
typedef unsigned int uint32_t;

const char SERVER_IP[]{"127.0.0.1"};
const int SERVER_PORT{12345};
// kcp 发送最低时间间隔
const int KCP_UPDATE_INTERVAL{5}; // ms
const int MAX_MSG_SIZE{1024 * 10}; // 10KB


const std::string KCP_CONNECT_PACKET("kcp_connection_packet");
const uint32_t NOT_KCP_CONNECT_PACK{2^32-1};
const std::string KCP_SEND_CONV_PACKET("kcp_connection_back_packet conv:");
const std::string KCP_DISCONNECT_PACKET("kcp_disconnect_packet");

namespace KCP {
    // 回调事件类型
    enum eEventType {
        eSocketFailed = -1,     // 构造函数中创建套接字失败，通过sendMsg回调
        eConnect = 0,           // kcp连接成功回调
        eConnectFailed,         // kcp连接失败回调
        eDisconnect,            // 服务端关闭或异常
        eRecvMsg                // kcp收到消息后解析后raw msg回调
    };
    // 回调函数类型
    typedef void(client_event_callback_t)(uint32_t, eEventType, const std::string&, void*);
    bool isRequireConnect(const char* buffer, int len);
    std::string generateConnectMsg(uint32_t conv);
    uint32_t getKcpConv(const char* buffer, int len);
};

#define KCP_CONNECTED 0
#define SUCCESS 0


#define KCP_ERR_CREATE_SOCKET_FAIL -2000
#define KCP_ERR_GET_SOCK_FLAG_FAIL -2001
#define KCP_ERR_SET_NON_BLOCK_FAIL -2002
#define KCP_ERR_ADDRESS_INVALID -2003
#define KCP_ERR_CONNECT_FAILED -2004
#define KCP_ERR_SEND_FAILED -2005
#define KCP_ERR_RECV_FAILED -2006
#define KCP_ERR_RECV_CONV_FAILED -2007

#define KCP_ERR_CREATE_KCPCB_FAILED -3000
#define KCP_ERR_CONNECT_TIMEOUT -3001