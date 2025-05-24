#include "../include/connection_container.hpp"
#include "../include/connection.hpp"

#include <iostream>

namespace KCP
{

connection_container::connection_container() {

}

std::shared_ptr<connection> connection_container::findByConv(const uint32_t& conv) {
    auto iter = connections_.find(conv);
    if (iter == connections_.end()) {
        return std::shared_ptr<connection>();
    } else {
        return iter->second;
    };
}

void connection_container::update(uint32_t clock) {
    for (auto iter = connections_.begin(); iter != connections_.end();) {
        std::shared_ptr<connection> conn = iter->second;
        conn->update(clock);
        if (conn->isTimeout()) {
            conn->doTimeout();
            connections_.erase(iter++);
        } else 
            ++iter;
    }
}

void connection_container::stop() {
    connections_.clear();
}

    
std::shared_ptr<connection> connection_container::addConnection(std::weak_ptr<connection_manager> manager, const uint32_t conv, const struct sockaddr_in* addr) {
    std::shared_ptr<connection> conn = connection::create(manager, conv, addr);
    if (conn) {
        connections_[conv] = conn;
        std::cout << "add connection conv: " << conv << std::endl;
    }
    return conn;
}

void connection_container::removeConnection(const uint32_t& conv) {
    connections_.erase(conv);
}

    
uint32_t connection_container::getNewConv() const {
    static uint32_t s_cur_conv = 1000;
    return ++s_cur_conv;
}


}; // namespace KCP
