#pragma once
using namespace std;

#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include "socketchannel.h"

class SocketController : public ISocketController
{
private:
    // Map of host names to SocketChannel objects for efficient lookup
    unordered_map<string, shared_ptr<SocketChannel>> _channels;

    // Mutex to protect access to the channel map
    mutable mutex _mutex;

    // Adds a new SocketChannel to the controller
    void AddChannel(const string& hostName, const string& friendlyName, uint16_t port = 49152)
    {
        auto newChannel = make_shared<SocketChannel>(hostName, friendlyName, port);
        {
            lock_guard<mutex> lock(_mutex);
            _channels[hostName] = newChannel;
        }
        newChannel->Start();
    }

public:
    SocketController() = default;
    ~SocketController() { StopAll(); }

    void AddChannelsForCanvases(const vector<shared_ptr<ICanvas>> &allCanvases) override
    {
        for (const auto &canvas : allCanvases)
        {
            for (const auto &feature : canvas->Features())
                AddChannel(feature->HostName(), feature->FriendlyName(), feature->Port());
            canvas->Effects().Start(*canvas, *this);
        }
    }
    
    // Finds a channel by host name
    shared_ptr<ISocketChannel> FindChannelByHost(const string& hostName) const override
    {
        lock_guard<mutex> lock(_mutex);
        auto it = _channels.find(hostName);
        if (it != _channels.end())
            return it->second;
        
        return nullptr; // Return null if no matching channel is found
    }

    // Starts all channels
    void StartAll() override
    {
        lock_guard<mutex> lock(_mutex);
        for (auto& [_, channel] : _channels)
            channel->Start();
        
    }

    // Stops all channels
    void StopAll() override
    {
        lock_guard<mutex> lock(_mutex);
        for (auto& [_, channel] : _channels)
            channel->Stop();
    }

        // Removes all channels
    void RemoveAllChannels() override
    {
        StopAll();
        lock_guard<mutex> lock(_mutex);
        _channels.clear();
    }

};
