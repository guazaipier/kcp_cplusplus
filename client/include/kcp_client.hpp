#pragma once

#include <string>
#include <thread>
#include <future>
#include <mutex>
#include <atomic>
#include "util.h"

namespace KCP
{
    class KcpClient
    {
    public:
        KcpClient(const std::string& ip, const int port);
        KcpClient() = delete;
        ~KcpClient();

        // 设置上层回调，进行事件通知
        void set_event_callback(const client_event_callback_t& event_callback_func, void* var);
        // 请求建立kcp连接；当失败会通过run回调给上层，上层重新调用这个函数请求连接,，建立kcp连接
        int run();

        void exit();

        // 应用层直接调用，通过kcp发送数据
        void send(const std::string& msg);

    private:
        void requireKcpConv();
        // 开启kcp
        void transferKcp(std::promise<int>& prom);

        void start();
        void stop();

        // 发送客户端数据
        void updateInLoop();
        // 接收数据
        void recvInLoop();

        // 处理从服务端接收到的消息，剔除kcp头部，解析出来的消息通过事件回调给上层
        void processMsg(const std::string& msg);

    private:
        // 初始化套接字
        int initUdpConnect();
        // 设置非阻塞套接字，方便正常退出run
        void setNonblock();
        
        // 初始化kcp
        int initKcp(unsigned int conv);
        // kcp内部用于发送数据的函数
        static int kcpOutput(const char *buf, int len, ikcpcb* kcp, void *user);   

    private:
        int sock_fd_{-1};
        std::string server_ip_;
        int server_port_;

        ikcpcb* kcp_{};
        std::mutex kcp_mtx_;
        
        std::atomic_bool running_{false};
        std::thread thread_[2];

        // 事件通知上层的回调函数
        client_event_callback_t* pevent_func_{};
        void* pevent_func_val_{};
    };
};