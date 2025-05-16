#pragma once

#include "util.hpp"
#include <chrono>

namespace KCP {

class connection_manager;

class connection {
public:
    connection(const std::weak_ptr<connection_manager>&);
    ~connection();

    static std::shared_ptr<connection> create(const std::weak_ptr<connection_manager>&, const uint32_t conv, const struct sockaddr_in* addr);

    void input(const std::string& msg);
    void send(const std::string& msg);
    void update(uint32_t clock);

    bool isTimeout() const;
    void doTimeout();

private:
    void initKcp(const uint32_t& conv);
    void clear();

    static int kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user);
    void sendUdpMsg(const char* buf, int len);
    
    uint32_t getCurClock() const;

private:
    std::weak_ptr<connection_manager> connection_manager_;  // 通过上层的弱引用使用socket功能
    struct sockaddr_in addr_{};
    ikcpcb* kcp_{nullptr};
    uint32_t conv_{0};                 // kcp的conv头部
    uint32_t last_recv_msg_clock_{0};   // 用于计算客户端是否已经超时关闭
};

};