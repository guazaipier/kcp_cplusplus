#include "kcp_client.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <functional>
#include <sstream>
#include <future>
#include <exception>

#include "ikcp.h"

namespace KCP
{
KcpClient::KcpClient(const std::string& ip, const int port) {
    server_ip_ = ip, server_port_ = port;
    initUdpConnect();
}

KcpClient::~KcpClient() {
    exit();
}

void KcpClient::set_event_callback(const client_event_callback_t& event_callback_func, void* val) {
    pevent_func_ = &event_callback_func;
    pevent_func_val_ = val;
}

int KcpClient::run() {
    if (running_) { return SUCCESS; };
    
    std::promise<int> prom;
    std::future<int> fut = prom.get_future();
    //     std::function<void()> update_task([this] {this->updateInLoop();});
    // thread_ = std::thread(std::move(update_task));
    std::function<void(std::promise<int>&)> conv_task([this] (std::promise<int>& prom) {this->transferKcp(prom);});
    std::thread get_conv_thread(std::move(conv_task), std::ref(prom));

    requireKcpConv();

    // 建立kcp链接
    auto ret = fut.get();
    if(get_conv_thread.joinable())
        get_conv_thread.join();  
    if (ret != SUCCESS) { 
        if (pevent_func_) {
            pevent_func_(0, eConnectFailed, std::to_string(ret), pevent_func_val_);
        }
        std::cout << "failed to connect kcp server..." << ret << std::endl;
        return ret; 
    }
    
    start();

    std::function<void()> update_task([this] {this->updateInLoop();});
    thread_[0] = std::thread(std::move(update_task));

    std::function<void()> run_task([this] {this->recvInLoop();});
    thread_[1] = std::thread(std::move(run_task));

    std::cout << "connect success!" << std::endl;
    return SUCCESS;
}

void KcpClient::send(const std::string& msg) {
    std::cout << "send msg: " << msg << std::endl;
    {
        std::unique_lock<std::mutex> lock(kcp_mtx_);
        int ret = ikcp_send(kcp_, msg.c_str(), msg.size());
        if (ret < 0) {
            std::cerr << "ikcp_send error return with errno: " << ret << std::endl; 
        }
    }
}

void KcpClient::updateInLoop() {
    std::cout << "thread_update start: " << std::this_thread::get_id() << std::endl;

    auto last = std::chrono::system_clock::now();

    while (running_) {
        auto current = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(current-last).count() >= KCP_UPDATE_INTERVAL || (current < last)) {
            std::unique_lock<std::mutex> lock(kcp_mtx_);
            if (!kcp_) return;
            ikcp_update(kcp_, current.time_since_epoch().count());
            last = current;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // sleep 1ms
    }
    std::cout << "thread_update exit." << std::endl;
}

void KcpClient::recvInLoop() {
    std::cout << "thread_recvInLoop start: " << std::this_thread::get_id() << std::endl;

    setNonblock();
    
    char buffer[MAX_MSG_SIZE]{};
    while (running_){
        try {
            const ssize_t len = ::recv(sock_fd_, buffer, sizeof(buffer), 0);
            if (len > 0) {
                processMsg(std::string(buffer, len));
            }
        } catch (std::exception e) {
            std::ostringstream ossm;
            ossm << "recv error with errno: " << errno << " " << strerror(errno);
            std::cout << ossm.str() << std::endl;
            if (pevent_func_)
                pevent_func_(kcp_->conv, eDisconnect, ossm.str(), pevent_func_val_);
            return;
        }
    }

    std::cout << "thread_recvInLoop exit." << std::endl;
}

void KcpClient::exit() {
    stop();
    // wait for thread exit normally.
    for (int i = 0; i < 2; ++i)
        if(thread_[i].joinable()) 
            thread_[i].join();

    {
        std::unique_lock<std::mutex> lock(kcp_mtx_);
        if (kcp_) {
            ::ikcp_release(kcp_);
            kcp_ = nullptr;
        }        
    }

    if (sock_fd_) {
        ::close(sock_fd_);
        sock_fd_ = 0;
    }
}

void KcpClient::requireKcpConv() {
    std::cout << "send packet: " << KCP_CONNECT_PACKET << std::endl;
    const ssize_t ret = ::send(sock_fd_, KCP_CONNECT_PACKET.c_str(), KCP_CONNECT_PACKET.length(), 0);
    if (ret < 0) {
        std::cerr << "send require kcp conv failed with errno: " << errno << " " << strerror(errno) << std::endl;
    }
}

void KcpClient::transferKcp(std::promise<int>& prom) {
    std::cout << "wait for kcp conv back..." << std::endl;
    char buffer[1400]{};

    const ssize_t len = ::recv(sock_fd_, buffer, sizeof(buffer), 0);
    if (len < 0 && len != EAGAIN) {
        std::cerr << "recv error with errno: " << errno << " " << strerror(errno) << std::endl;
        prom.set_value(KCP_ERR_RECV_CONV_FAILED);
        return;
    }
    
    std::cout << "recv conv packet: " << buffer << " len: " << len << std::endl;
    auto conv = getKcpConv(buffer, len);
    if (conv == NOT_KCP_CONNECT_PACK) {
        std::cerr << "recv conv error while parse failed." << std::endl;
        prom.set_value(KCP_ERR_RECV_CONV_FAILED);
        return;
    }

    std::cout << "get conv: " << conv << std::endl;
    int ret = initKcp(conv);
    if (SUCCESS != ret) {
        std::cerr << "create kcp error with errno: " << ret << std::endl;
        prom.set_value(ret);
        return;
    }

    std::cout << "kcp connected." << std::endl;
    prom.set_value(SUCCESS);
}

void KcpClient::start() {
    running_ = true;
}

void KcpClient::stop() {
    running_ = false;
}

void KcpClient::processMsg(const std::string& recv_buffer) {
    {    
        std::unique_lock<std::mutex> lock(kcp_mtx_);
        ikcp_input(kcp_, recv_buffer.c_str(), recv_buffer.length());
    }
    while (true) {
        char buffer[MAX_MSG_SIZE]{};
        int len = 0;
        {
            std::unique_lock<std::mutex> lock(kcp_mtx_);
            len = ikcp_recv(kcp_, buffer, sizeof(buffer));
        }
        if (len < 0) {
            break;
        }
        std::string msg(buffer, len);
        std::cout << "recv kcp msg: " << msg << std::endl;
        if (pevent_func_) {
            pevent_func_(kcp_->conv, eRecvMsg, msg, pevent_func_val_);
        }
    }
}

int KcpClient::initUdpConnect() {
    // 创建套接字
    {
        sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd_ < 0) {
            std::cerr << "socket error return with errno: " << errno << " " << strerror(errno) << std::endl;
            return KCP_ERR_CREATE_SOCKET_FAIL;
        }
    }

    struct sockaddr_in server_addr{};
    {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port_);
        if (::inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "inet_pton error return <= 0, with errno: " << errno << " " << strerror(errno) << std::endl;
            return KCP_ERR_ADDRESS_INVALID;
        }
        // ::bind(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
    // 连接server
    {
        int ret = ::connect(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr));
        if (ret < 0) {
            std::cerr << "connect error return with errno: " << errno << " " << strerror(errno) << std::endl;
            return KCP_ERR_CONNECT_FAILED;
        }
    }
    return SUCCESS;
}

void KcpClient::setNonblock() {
    // 确保正常退出run中的::recv
    int flag = ::fcntl(sock_fd_, F_GETFL, 0);
    if (flag < 0) {
        std::cerr << "fcntl getfl failed with errno " << errno << " " << strerror(errno) << std::endl;
    } else {
        int ret = ::fcntl(sock_fd_, F_SETFL, flag | O_NONBLOCK);
        if (ret < 0) {
            std::cerr << "fcntl setfl failed with errno " << errno << " " << strerror(errno) << std::endl;
        }
    }
}

int KcpClient::initKcp(unsigned int conv) {
    kcp_ = ikcp_create(conv, (void*)this);
    if (!kcp_) {
        return KCP_ERR_CREATE_KCPCB_FAILED;
    }
    ikcp_nodelay(kcp_, 1, KCP_UPDATE_INTERVAL, 2, 1);
    kcp_->output = kcpOutput;

    return SUCCESS;
}

int KcpClient::kcpOutput(const char *buf, int len, ikcpcb* kcp, void *user) {
    ssize_t ret = ::send(((KcpClient*)user)->sock_fd_, buf, len, 0);
    if (ret < 0) {
        std::cerr << "send error with errno: " << errno << " " << strerror(errno) << std::endl;
        return KCP_ERR_SEND_FAILED;
    }
    

    // ikcp_send_msg_check(((KcpClient*)user)->kcp_, buf, len);
    // std::cout << "send msg: " << std::string(buf,len) << " len: " << len << std::endl;

    return 0;
}

};