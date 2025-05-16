#pragma once

#include "util.hpp"
#include <unordered_map>

namespace KCP {

class connection_manager;
class connection;

class connection_container {
public:
    connection_container();
    std::shared_ptr<connection> findByConv(const uint32_t& conv);

    void update(uint32_t clock);
    void stop();
    
    std::shared_ptr<connection> addConnection(std::weak_ptr<connection_manager> manager, const uint32_t conv, const struct sockaddr_in* addr);
    void removeConnection(const uint32_t& conv);
    
    uint32_t getNewConv() const;

private:
    std::unordered_map<uint32_t, std::shared_ptr<connection>> connections_;
};

};