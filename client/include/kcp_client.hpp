#pragma once

#include <string>
#include <thread>
#include <future>
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
        int connect();
        // 获取服务端发送过来的数据，发送客户端数据
        void run();
        void exit();

        // 应用层直接调用，通过kcp发送数据
        void sendMsg(const std::string& msg);

    private:
        void requireKcpConv();
        void transferKcp(std::promise<int>& prom);

        void start();
        void stop();

        int initKcp(unsigned int conv);

        // 发送客户端数据
        void updateInLoop();

        // 处理从服务端接收到的消息，剔除kcp头部，解析出来的消息通过事件回调给上层
        void processMsg(const std::string& msg);

        // kcp内部用于发送数据的函数
        static int kcpOutput(const char *buf, int len, ikcpcb* kcp, void *user);

    private:
        // 初始化套接字
        int initUdpConnect();
        // 设置非阻塞套接字，方便正常退出run
        void setNonblock();

    private:
        int sock_fd_{-1};
        std::string server_ip_;
        int server_port_;

        ikcpcb* kcp_{};
        
        bool is_connected_{false};
        bool is_in_kcp_{false};

        std::thread thread_;

        // 事件通知上层的回调函数
        client_event_callback_t* pevent_func_{};
        void* pevent_func_val_{};
    };
};