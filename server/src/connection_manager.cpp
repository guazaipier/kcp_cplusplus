#include "../include/connection_manager.hpp"

// time & net files
#include <chrono>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>

// stream files
#include <iostream>
#include <cstring>
#include <assert.h>
#include <signal.h>

#include "../include/ikcp.h"
#include "../include/connection_container.hpp"
#include "../include/connection.hpp"

namespace KCP {

void signalDisable() {
    // ignore the child terminate signal
    signal(SIGCHLD, SIG_IGN);
    // 忽略由于写操作的错误而导致的程序退出
    signal(SIGPIPE, SIG_IGN);
}

connection_manager::connection_manager(const int port) : connection_(std::make_unique<connection_container>()) {
    std::cout << "port: " << port << std::endl;
    initServer(port);
    if (!sockfd_) return;

    {
        // 开启kcp缓冲区定时刷新
        std::function<void()> update_task([this]{ this->update(); });
        threads_.push_back(std::thread(std::move(update_task)));

        // 开启处理接收到的消息
        std::function<void()> process_recv_msg_task([this]{ this->recv(); });
        threads_.push_back(std::thread(std::move(process_recv_msg_task)));
        // std::function<void()> process_recv_task = std::bind(&connection_manager::recv, this);
        // threads_.push_back(std::thread(process_recv_task));
    }

    signalDisable();
}

connection_manager::~connection_manager() {
    if (!stopped_) {
        stop();
    }
}

void connection_manager::run() {
    std::cout << "kcp server start running..." << std::endl;
    struct sockaddr_in addr{};
    socklen_t addr_len = sizeof(addr);
    while (!stopped_) {
        epoll_event events[SOMAXCONN];
        int nfds = epoll_wait(epoll_fd_, events, SOMAXCONN, 10);
        if (nfds == -1) {
            if (errno == EINTR) {
                std::cout << "epoll_wait interrupted by signal" << std::endl; // gdb ctrl+c
                continue;
            } else {
                std::cout << "epoll_wait error" << std::endl;
                break;
            }
        } else if (nfds == 0) {
            continue; //timeout todo something not busy
        } else {
            for (int i = 0; i < nfds; ++i) {
                if (events[i].data.fd == sockfd_) {
                    while (true) { // et 模式必须一次性读完，防止丢包
                        std::vector<char> recv_data(MAX_MSG_SIZE, '\0');
                        int recv_len = ::recvfrom(sockfd_, recv_data.data(), recv_data.size(), 0, (struct sockaddr*)&addr, &addr_len);
                        if (recv_len > 0) {
                            std::pair<std::string, struct sockaddr_in> msg(std::string(recv_data.data(), recv_len), addr);
                            std::unique_lock<std::mutex> msg_lock(mtx_);
                            recv_que_.push(msg);
                            cv_.notify_one();
                        } else if (recv_len == 0) {
                            break;
                        }
                    }

                }
            }
        }
    }

    // while (!stopped_) {
    //     recv_len = ::recvfrom(sockfd_, recv_data, sizeof(recv_data), 0, (struct sockaddr*)&addr, &addr_len);
    //     if (recv_len > 0) {
    //         std::pair<std::string, struct sockaddr_in> msg(std::string(recv_data, recv_len), addr);
    //         std::unique_lock<std::mutex> msg_lock(mtx_);
    //         recv_que_.push(msg);
    //         cv_.notify_one();
    //     }
    // }
    std::cout << "run exit." << std::endl;
}

// stop
void connection_manager::stop() {
    std::cout << "kcp_stop start: " << std::endl;
    stopped_.store(true);
    connection_->stop();
    if (sockfd_ > 0) {
        ::close(sockfd_);
        sockfd_ = 0;
    }

    for (auto iter = threads_.begin(); iter != threads_.end(); ++iter) {
        if (iter->joinable())
            iter->join();
    }
    std::cout << "kcp stopped." << std::endl;
}

// timeout to disconnect client.
void connection_manager::forceDisconnect(const uint32_t& conv) {
    std::cout << "force disconnect: " << conv << std::endl;

    if (!connection_->findByConv(conv))
        return;
    
    std::shared_ptr<std::string> msg(new std::string("server force disconnect"));
    callCallBack(conv, eEventType::eDisconnect, msg);

   connection_->removeConnection(conv);
}

void connection_manager::setCallback(const std::function<event_callback_t>& func) {
    event_callback = func;
}

// send by kcp
int connection_manager::send(const uint32_t& conv, std::shared_ptr<std::string> msg) {
    std::shared_ptr<KCP::connection> conn =connection_->findByConv(conv);
    if (!conn)
        return KCP_ERR_NOT_EXIST_CONNECTION;
    
   conn->send(*msg);
   return 0;
}

// send by udp
void connection_manager::sendByUdp(const char* buf, int len, struct sockaddr_in& addr) {
    int ret = ::sendto(sockfd_, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        std::cout << "send failed with errno " << errno << " " << strerror(errno) << std::endl;
        return;
    }
    // std::cout << "send: " << buf << " len: " << len << " addr: " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << std::endl;
}
    
void connection_manager::callCallBack(const uint32_t conv, eEventType event_type, std::shared_ptr<std::string> msg) {
    event_callback(conv, event_type, msg);
}

// 单独做一个线程，与::recv分开
void connection_manager::recv() {
    std::cout << "thread_recv start: " << std::this_thread::get_id() << std::endl;

    std::pair<std::string, struct sockaddr_in> recv_msg;
    while (!stopped_) {
        {
            std::unique_lock<std::mutex> recv_lock(mtx_);
            cv_.wait_until(recv_lock, std::chrono::system_clock::now() + std::chrono::milliseconds(1));
        }
        while (!recv_que_.empty()) {
            recv_msg = recv_que_.front();
            recv_que_.pop();
            if (0 == isRequireConnect(recv_msg.first.c_str(), recv_msg.first.length()))
                processConnection(&(recv_msg.second));
            else 
                processKcpMsg(recv_msg.first); // TODO: working thread pool to handle kcp msg.
        }
    }
    
    std::cout << "thread_recv exit.";
}

void connection_manager::update() {
    std::cout << "thread_update start: " << std::this_thread::get_id() << std::endl;
    while (!stopped_) {
        auto current = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        cur_clock_.store(current);
        connection_->update(current);
        std::this_thread::sleep_for(std::chrono::milliseconds(KCP_UPDATE_INTERVAL));
    }
    
    std::cout << "thread_update exit.";
}

            
void connection_manager::processConnection(struct sockaddr_in* addr) {
    uint32_t conv = connection_->getNewConv();
    std::string send_back_msg = GenerateSendBackConvMsg(conv);
    int ret = ::sendto(sockfd_, send_back_msg.c_str(), send_back_msg.length(), 0, (struct sockaddr*)addr, sizeof(*addr));
    if (ret < 0) {
        std::cout << "send failed with errno " << errno << " " << strerror(errno) << std::endl;
        return;
    }
    std::cout << "send: " << send_back_msg << " addr: " << inet_ntoa(addr->sin_addr)<< ":" << ntohs(addr->sin_port) << std::endl;
    connection_->addConnection(shared_from_this(), conv, addr);
}

void connection_manager::processKcpMsg(const std::string& recv_msg) {
    std::cout << "recv msg len: " << recv_msg.length() << " " << recv_msg.c_str() + IKCP_OVERHEAD << std::endl;
    // ikcp_send_msg_check(recv_msg.c_str(), recv_msg.length());
    uint32_t conv = ikcp_getconv(recv_msg.c_str());
    // std::cout << "get_conv: " << conv << std::endl;
    auto conn = connection_->findByConv(conv);
    if (!conn) {
        std::cout <<  "connection not exist with conv: " << conv << std::endl;
        return;
    }

    conn->input(recv_msg);
}

void connection_manager::initServer(const int& port) {
    // create socket
    {
        sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd_ <= 0) { 
            std::cerr << "create socket failed with errno " << errno << " " << strerror(errno) << std::endl;
            return; 
        }
    }
    // nonblock
    {
        int flags = fcntl(sockfd_, F_GETFL, 0);
        if (flags == -1) {
            std::cerr << "get socket non-blocking: fcntl error return with errno: " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
            return;
        }
        if(fcntl(sockfd_, F_SETFL, flags | O_NONBLOCK) == -1) {
            std::cerr << "set socket non-blocking: fcntl error return with errno: " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
            return;
        }
    }
    // set addr reuse
    {
        int on = 1;
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
            std::cerr << "set socket reuse addr failed with errno " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
            return;
        }
        if (::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) == -1) {
            std::cerr << "set socket reuse port failed with errno " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
            return;
        }
    }
    // bind addr
    {
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (::bind(sockfd_, (struct sockaddr*)&addr,sizeof(addr)) == -1) {
            std::cerr << "bind addr failed with errno " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
            return;
        }
    }
    // set epoll
    {
        epoll_fd_ = epoll_create(1);
        if (epoll_fd_ == -1) {
            std::cerr << "create epoll failed with errno " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
            return;
        }
        epoll_event event{};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = sockfd_;
        if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sockfd_, &event) == -1) {
            std::cerr << "add epoll failed with errno " << errno << " " << strerror(errno) << std::endl;
            ::close(sockfd_);
            sockfd_ = 0;
        }
    }
}

};