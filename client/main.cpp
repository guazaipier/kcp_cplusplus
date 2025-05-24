#include "kcp_client.hpp"
#include "util.h"

#include <thread>
#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <functional>
#include <signal.h>
#include <atomic>


namespace KCP
{
void event_call_back(uint32_t conv, eEventType e_type, const std::string& msg, void* param) {
    switch(e_type){
        case eSocketFailed:
            std::cout << "failed to initial kcpclient, destroy KcpClient." << std::endl;
            // (KCP::KcpClient*)param)->exit();
            break;
        case eConnectFailed:
            std::cout << "failed to connect kcp, try connnect..." << std::endl;
            break;
        case eConnect:
            std::cout << "connect success from kcp client." << std::endl;
            break;
        case eDisconnect:
            std::cout << "closed by server." << std::endl;
            // (KCP::KcpClient*)param)->exit();
            break;
        case eRecvMsg:
            // std::cout << "recv msg from KcpClient: " << msg << std::endl;
            break;
    };
}

};

int main(int argc, char* argv[]) {

    KCP::KcpClient client(SERVER_IP, SERVER_PORT);
    client.set_event_callback(KCP::event_call_back, &client);

    int res = client.connect();
    if (res != SUCCESS) {
        res = client.connect();
        if (res != SUCCESS) {
            res = client.connect();
            if (res != SUCCESS) {
                return KCP_ERR_CONNECT_FAILED;
            }
        }
    }
    std::atomic_bool stopped{false};

    // 定时关闭client
    std::function<void()> timer_task = [&]{
        std::this_thread::sleep_for(std::chrono::seconds(60));
        stopped = true;
    };
    std::thread timer = std::thread(std::move(timer_task));

    {
        int64_t count = 0;
        while (!stopped) {
            client.send("hello world!");
            count++;
            // std::cout << "--------------------------" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        std::cout << "send count: " << count << std::endl;       
    }
 
    client.exit();
    timer.join();
    std::cout << "exit normally." << std::endl;
    return 0;
}