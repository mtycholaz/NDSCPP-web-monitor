#pragma once

// SocketController
//
// Manages the list of active SocketChannels.  This class is responsible for creating
// and destroying SocketChannels, and for starting and stopping them.  It also provides
// a method to find a SocketChannel by host name.

#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include "socketchannel.h"

class SocketController
{
private:
    std::vector<std::shared_ptr<SocketChannel>> _channels;
    mutable std::mutex _mutex; // Ensure this is not const

public:
    SocketController() = default;
    ~SocketController() { StopAll(); }

    // Adds a new SocketChannel to the controller
    void AddChannel(const std::string& hostName, const std::string& friendlyName, uint16_t port = 49152)
    {
        auto newChannel = std::make_shared<SocketChannel>(hostName, friendlyName, port);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _channels.push_back(newChannel);
        }
        newChannel->Start();
    }

    // Removes a SocketChannel by host name
    void RemoveChannel(const std::string& hostName)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = std::remove_if(_channels.begin(), _channels.end(),
                                 [&](const std::shared_ptr<SocketChannel>& channel) {
                                     return channel->HostName() == hostName;
                                 });
        _channels.erase(it, _channels.end());
    }

    void RemoveAllChannels()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _channels.clear();
    }
    
    // Finds a channel by host name
    std::shared_ptr<SocketChannel> FindChannelByHost(const std::string& hostName) const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& channel : _channels)
        {
            if (channel->HostName() == hostName)
                return channel;
        }
        return nullptr; // Return null if no matching channel is found
    }
    
    // Starts all channels
    void StartAll()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& channel : _channels)
            channel->Start();
    }

    // Stops all channels
    void StopAll()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& channel : _channels)
            channel->Stop();
    }
};
