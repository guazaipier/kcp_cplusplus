#pragma once

#include "util.h"
#include <memory>

namespace KCP
{
    // TODO：设置内部类，避免向客户端调用者暴露更多的实现细节，编译快速，向外提供.h .so文件
    class KcpClient
    {
    private:
        // 内部类，用于处理kcp的收发
        class Session;
    public:
        KcpClient(const std::string& ip, const int port);
        KcpClient() = delete;
        ~KcpClient();

        // 设置上层回调，进行事件通知
        void set_event_callback(const client_event_callback_t& event_callback_func, void* var);
        // 请求建立kcp连接；当失败会通过run回调给上层，上层重新调用这个函数请求连接,，建立kcp连接
        int connect();
        // 结束 kcp 连接
        void exit();

        // 应用层直接调用，通过kcp发送数据
        void send(const std::string& msg);

    private:
        std::unique_ptr<Session> session_;
    };
};