#pragma once

#include "util.hpp"
#include "connection_container.hpp"

#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <condition_variable>
#include <mutex>

namespace KCP {

class connection_manager : public std::enable_shared_from_this<connection_manager> {
public:
    connection_manager(const int port);
    ~connection_manager();

    void run();

    // stop
    void stop();

    // timeout to disconnect client.
    void forceDisconnect(const uint32_t& conv);

    void setCallback(const std::function<event_callback_t>& func);

    // send by kcp
    int send(const uint32_t& conv, std::shared_ptr<std::string> msg);
    // send by udp
    void sendByUdp(const char* buf, int len, struct sockaddr_in& addr);
    
    void callCallBack(const uint32_t conv, eEventType event_type, std::shared_ptr<std::string> msg);

    uint32_t getCurClock() const { return cur_clock_.load(); };
private:
    void recv();
    void update();

    void processConnection(struct sockaddr_in*);
    void processKcpMsg(std::string recv_msg);

private:
    void initServer(const int& port);

private:
    std::atomic<bool> stopped_{false};
    std::atomic<uint32_t> cur_clock_{};

    std::vector<std::thread> threads_;

    std::function<event_callback_t> event_callback;

    int sockfd_{0};

    std::queue<std::pair<std::string, struct sockaddr_in>> recv_que_;
    std::mutex mtx_;
    std::condition_variable cv_;

    connection_container connection_;
};

};