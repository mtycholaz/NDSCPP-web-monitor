#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include "interfaces.h"
#include "utilities.h"
#include "pixeltypes.h"

class SocketChannel : public ISocketChannel
{
public:
    SocketChannel(const std::string& hostName, const std::string& friendlyName, uint32_t width, uint32_t height,
                  uint32_t offset, uint16_t channel, bool redGreenSwap, uint16_t port = 49152)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _width(width),
          _height(height),
          _offset(offset),
          _channel(channel),
          _redGreenSwap(redGreenSwap),
          _port(port),
          _isConnected(false),
          _bytesPerSecond(0),
          _running(false)
    {}

    ~SocketChannel() override { Stop(); }

    // Override methods from ISocketChannel
    void Start() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running) {
            _running = true;
            _workerThread = std::thread(&SocketChannel::WorkerLoop, this);
        }
    }

    void Stop() override
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _running = false;
        }

        if (_workerThread.joinable())
            _workerThread.join();
    }

    bool IsConnected() const override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _isConnected;
    }

    uint32_t BytesPerSecond() const override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _bytesPerSecond;
    }

    void ResetBytesPerSecond() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _bytesPerSecond = 0;
    }

    virtual const std::string& HostName() const override { return _hostName; };
    virtual const std::string& FriendlyName() const override { return _friendlyName; };
    virtual uint32_t Width() const override { return _width; };
    virtual uint32_t Height() const override { return _height; };
    uint16_t Port() const override { return _port; }

    bool EnqueueFrame(const std::vector<uint8_t>& frameData, std::chrono::time_point<std::chrono::system_clock> timestamp) override
    {
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            if (_frameQueue.size() >= MaxQueueDepth)
                return false; // Queue is full
            _frameQueue.push(frameData);
        }
        return true; // Successfully enqueued
    }


private:
    // Worker thread method
    void WorkerLoop()
    {
        while (_running) {
            std::vector<uint8_t> frame;
            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                if (!_frameQueue.empty()) {
                    frame = _frameQueue.front();
                    _frameQueue.pop();
                }
            }

            if (!frame.empty())
                SendFrame(frame);

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Adjust based on requirements
        }
    }

    void SendFrame(const std::vector<uint8_t>& frame)
    {
        // Simulate sending data and updating the connection state
        try {
            std::lock_guard<std::mutex> lock(_mutex);
            _isConnected = true;
            _bytesPerSecond += frame.size();
        } catch (...) {
            std::lock_guard<std::mutex> lock(_mutex);
            _isConnected = false;
        }
    }

    std::vector<uint8_t> GetFrameData(const std::vector<CRGB>& leds, std::chrono::time_point<std::chrono::system_clock> timestamp) const
    {
        auto pixelData = Utilities::GetColorBytesAtOffset(leds, _offset, _width * _height, false, _redGreenSwap);

        // Calculate epoch time from timestamp
        auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch()).count();
        uint64_t seconds = epoch / 1'000'000;
        uint64_t microseconds = epoch % 1'000'000;

        return Utilities::CombineByteArrays({
            Utilities::WORDToBytes(CommandPixelData),
            Utilities::WORDToBytes(_channel),
            Utilities::DWORDToBytes(static_cast<uint32_t>(pixelData.size() / 3)),
            Utilities::ULONGToBytes(seconds),
            Utilities::ULONGToBytes(microseconds),
            pixelData
        });
    }

private:
    // Constants
    static constexpr uint16_t CommandPixelData = 3;
    static constexpr size_t MaxQueueDepth = 100;

    // Member variables
    std::string _hostName;
    std::string _friendlyName;
    uint32_t _width;
    uint32_t _height;
    uint32_t _offset;
    uint16_t _channel;
    bool _redGreenSwap;
    uint16_t _port;

    mutable std::mutex _mutex;       // Protects connection state
    std::atomic<bool> _isConnected;
    std::atomic<uint32_t> _bytesPerSecond;
    std::atomic<bool> _running;

    std::mutex _queueMutex;          // Protects the frame queue
    std::queue<std::vector<uint8_t>> _frameQueue;

    std::thread _workerThread;
};
