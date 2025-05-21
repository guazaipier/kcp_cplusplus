#include "include/connection_manager.hpp"

#include <iostream>
#include <signal.h>
#include <atomic>

std::atomic<bool> g_stopped{};

void signalHandler(int sig_num);
void signalManager();

int main(int argc, char* argv[]) {
    signalManager();
    try {
        // // cmd params
        // if (argc != 3) {
        //     std::cerr << "Usage: -port <port>" << std::endl;
        //     return 1;
        // }
        
        // int port = atoi(argv[2]);
        
        std::shared_ptr<KCP::connection_manager> server(std::make_shared<KCP::connection_manager>(12345));
        if (!server->prepared()) { 
            std::cout << "server prepare failed." << std::endl; 
            return -1;
        }
        server->setCallback([&server](uint32_t conv, KCP::eEventType etype, std::shared_ptr<std::string> msg) {
            std::cout << "recv msg: " << etype << " " << msg->c_str() << std::endl;
            if (etype == KCP::eRecvMsg) {
                server->send(conv, msg);
            }
        });

        std::thread stop_wg([&server]{
            std::cout << "thread_signal start: " << std::this_thread::get_id() << std::endl;
            while(!g_stopped.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            server->stop();
            std::cout << "sig stopped." << std::endl;
        });

        server->run();

        stop_wg.join();
    } catch (std::exception& e) {
        std::cerr << "exception catched: " << e.what() << std::endl;
    }

    return 0;
}


void signalHandler(int sig_num) {
    std::cout << "caught signal: " << sig_num << std::endl;
    g_stopped.store(true);
}

void signalManager() {
        // // 终端退出信号，通常由 Ctrl+\ 触发,用于调试或特殊退出
    signal(SIGQUIT, signalHandler);
    // 捕获 SIGINT 和 SIGTERM，用于优雅关闭 socket、保存状态、清理资源等。
    // 终端中断信号，通常由 Ctrl+C 触发。用于优雅退出。
    signal(SIGINT, signalHandler);
    // 终止信号，常用于 kill 命令。用于优雅退出。
    signal(SIGTERM, signalHandler);
}

