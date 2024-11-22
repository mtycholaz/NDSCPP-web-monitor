#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include "socketchannel.h"

class SocketController : public ISocketController
{
private:
    // Map of host names to SocketChannel objects for efficient lookup
    std::unordered_map<std::string, std::shared_ptr<SocketChannel>> _channels;

    // Mutex to protect access to the channel map
    mutable std::mutex _mutex;

public:
    SocketController() = default;
    ~SocketController() { StopAll(); }

    void AddChannelsForCanvases(const vector<shared_ptr<ICanvas>> &allCanvases) override
    {
        for (const auto &canvas : allCanvases)
        {
            for (const auto &feature : canvas->Features())
                AddChannel(feature->HostName(), feature->FriendlyName());
            canvas->Effects().Start(*canvas, *this);
        }
    }
    
    // Adds a new SocketChannel to the controller
    void AddChannel(const std::string& hostName, const std::string& friendlyName, uint16_t port = 49152) override
    {
        auto newChannel = std::make_shared<SocketChannel>(hostName, friendlyName, port);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _channels[hostName] = newChannel;
        }
        newChannel->Start();
    }

    // Removes a SocketChannel by host name
    void RemoveChannel(const std::string& hostName) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _channels.find(hostName);
        if (it != _channels.end())
        {
            it->second->Stop();
            _channels.erase(it);
        }
    }

    // Finds a channel by host name
    std::shared_ptr<ISocketChannel> FindChannelByHost(const std::string& hostName) const override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _channels.find(hostName);
        if (it != _channels.end())
            return it->second;
        
        return nullptr; // Return null if no matching channel is found
    }

    // Starts all channels
    void StartAll() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& [_, channel] : _channels)
            channel->Start();
        
    }

    // Stops all channels
    void StopAll() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& [_, channel] : _channels)
            channel->Stop();
    }

        // Removes all channels
    void RemoveAllChannels() override
    {
        StopAll();
        std::lock_guard<std::mutex> lock(_mutex);
        _channels.clear();
    }

};
