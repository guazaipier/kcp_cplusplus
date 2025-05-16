#include "../include/connection.hpp"
#include "../include/ikcp.h"
#include "../include/connection_manager.hpp"

#include <iostream>
#include <memory>
#include <string.h>
#include <arpa/inet.h>

namespace KCP {

connection::connection(const std::weak_ptr<connection_manager>& manager) : connection_manager_(manager) {

}

connection::~connection() {
    clear();
}

std::shared_ptr<connection> connection::create(const std::weak_ptr<connection_manager>& manager, const uint32_t conv, const struct sockaddr_in* addr) {
    std::shared_ptr<connection> conn = std::make_shared<connection>(manager);
    if (conn) {
        conn->initKcp(conv);
        ::memcpy(&(conn->addr_), addr, sizeof(*addr));
        std::cout << "new connection from: " << inet_ntoa(addr->sin_addr)<< ":" << ntohs(addr->sin_port) << std::endl;
    }
    return conn;
}

void connection::input(const std::string& msg) {
    last_recv_msg_clock_ = getCurClock();

    ikcp_input(kcp_, msg.c_str(), msg.length());

    {
        char buffer[1024*1000]{};
        int rcv_len = ikcp_recv(kcp_, buffer, sizeof(buffer));
        if (rcv_len <= 0) {
            // std::cout << "kcp_recv_len" << rcv_len << " <= 0" << std::endl;
        } else {
            const std::string msg_packet(buffer, rcv_len);
            if (auto manager = connection_manager_.lock()) 
                manager->callCallBack(conv_, eRecvMsg, std::make_shared<std::string>(msg_packet));
            std::cout << "conv: " << conv_ << " time: " << last_recv_msg_clock_ << " recv: " << msg_packet << std::endl;
        }
    }
}

void connection::send(const std::string& msg) {
    int ret = ikcp_send(kcp_, msg.c_str(), msg.length());
    if (ret < 0) {
        std::cout << "send ret < 0: " << ret << std::endl;
    }
}

void connection::update(uint32_t clock) {
    ikcp_update(kcp_, clock);
}

bool connection::isTimeout() const {
    if (last_recv_msg_clock_ == 0) { return false; }

    return  (getCurClock() - last_recv_msg_clock_) >= KCP_CONNECTION_TIMEOUT_DEADLINE;
}

void connection::doTimeout() {
    if (auto manager = connection_manager_.lock()) {
        std::shared_ptr<std::string> msg(new std::string("timeout"));
        manager->callCallBack(conv_, eDisconnect, msg);
    }
}


void connection::initKcp(const uint32_t& conv) {
    conv_ = conv;

    kcp_ = ikcp_create(conv_, (void*)this);
    kcp_->output = &connection::kcpOutput;

    ikcp_nodelay(kcp_, 1, KCP_UPDATE_INTERVAL, 1, 1);// 设置成1次ACK跨越直接重传, 这样反应速度会更快. 内部时钟5毫秒.
}

void connection::clear() {
    std::cout << "clear connection conv: " << conv_ << std::endl;
    std::string disconnect_msg = GenerateDisconnectMsg(conv_);
    sendUdpMsg(disconnect_msg.c_str(), disconnect_msg.length());
    ikcp_release(kcp_);
    kcp_ = nullptr;
    conv_ = 0;
}

int connection::kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user) {
    ((connection*)user)->sendUdpMsg(buf, len);

    return 0;
}

void connection::sendUdpMsg(const char* buf, int len) {
    auto manager = connection_manager_.lock();
    manager->sendByUdp(buf, len, addr_);
}
    
uint32_t connection::getCurClock() const {
    if (auto manager = connection_manager_.lock()) {
        return manager->getCurClock();
    }
    return 0;
}

};