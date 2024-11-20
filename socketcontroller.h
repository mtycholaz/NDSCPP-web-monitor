#pragma once

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
    void AddChannel(const std::string& hostName, const std::string& friendlyName, uint32_t width, uint32_t height,
                    uint32_t offset, uint16_t channelIndex, bool redGreenSwap, uint16_t port = 49152)
    {
        auto newChannel = std::make_shared<SocketChannel>(hostName, friendlyName, width, height, offset, channelIndex, redGreenSwap, port);
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
        _channels.clear();
    }

    // Gets the total bytes per second across all channels
    uint32_t TotalBytesPerSecond() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        uint32_t total = 0;
        for (const auto& channel : _channels)
            total += channel->BytesPerSecond();
        return total;
    }
};
