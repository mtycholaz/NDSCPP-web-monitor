#pragma once

#include <memory>
#include <vector>
#include <mutex>
#include "canvas.h"
#include "socketcontroller.h"
#include "server.h"

class SystemController
{
public:
    SystemController()
        : _canvas(std::make_shared<Canvas>(100, 100)), // Default canvas size
          _socketController(std::make_shared<SocketController>())
    {
    }

    ~SystemController()
    {
        Stop();
    }

    // Starts all subsystems
    void Start()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _socketController->StartAll();
        _running = true;
    }

    // Stops all subsystems
    void Stop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running) return;

        _socketController->StopAll();
        _running = false;
    }

    // Adds a new canvas feature
    void AddFeatureToCanvas(std::shared_ptr<ILEDFeature> feature)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _canvas->AddFeature(feature);
    }

    // Removes a canvas feature by reference
    void RemoveFeatureFromCanvas(std::shared_ptr<ILEDFeature> feature)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _canvas->RemoveFeature(feature);
    }

    // Adds a new socket channel
    void AddSocketChannel(const std::string& hostName, const std::string& friendlyName, uint32_t width, uint32_t height,
                          uint32_t offset, uint16_t channelIndex, bool redGreenSwap, uint16_t port = 49152)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _socketController->AddChannel(hostName, friendlyName, width, height, offset, channelIndex, redGreenSwap, port);
    }

    // Removes a socket channel by host name
    void RemoveSocketChannel(const std::string& hostName)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _socketController->RemoveChannel(hostName);
    }

    // Gets the total bytes per second across all channels
    uint32_t GetTotalBytesPerSecond() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _socketController->TotalBytesPerSecond();
    }

    // Provides access to the canvas
    std::shared_ptr<Canvas> GetCanvas() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _canvas;
    }

    // Provides access to the socket controller
    std::shared_ptr<SocketController> GetSocketController() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _socketController;
    }

private:
    mutable std::mutex _mutex;
    bool _running = false;
    std::shared_ptr<Canvas> _canvas;
    std::shared_ptr<SocketController> _socketController;
};
