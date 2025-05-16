#include "kcp_client.hpp"
#include "util.h"

#include <thread>
#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <functional>
#include <signal.h>
#include <condition_variable>
#include <mutex>


std::condition_variable g_cv;
std::mutex g_mtx;

namespace KCP
{
void event_call_back(uint32_t conv, eEventType e_type, const std::string& msg, void* param) {
    switch(e_type){
        case eSocketFailed:
            std::cout << "failed to initial kcpclient, destroy KcpClient." << std::endl;
            // g_cv.notify_one();
            break;
        case eConnectFailed:
            std::cout << "failed to connect kcp, try connnect..." << std::endl;
            // 出去处理，避免client内部线程阻塞在这里
            break;
        case eConnect:
            std::cout << "connect success from kcp client." << std::endl;
            break;
        case eDisconnect:
            std::cout << "closed by server." << std::endl;
            // g_cv.notify_one();
            break;
        case eRecvMsg:
            std::cout << "recv msg from KcpClient: " << msg << std::endl;
            // g_cv.notify_one();
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

    std::function<void()> task = [&client] { client.run(); };
    std::thread g_running = std::thread(std::move(task));

    for (int i = 0; i < 3; ++i) {
        client.sendMsg("hello world!");
        std::cout << "--------------------------" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    client.exit();
    
    // {
    //     std::unique_lock<std::mutex> lock(g_mtx);
    //     g_cv.wait(lock);
    // }


    g_running.join();
    std::cout << "exit normally." << std::endl;
    return 0;
}