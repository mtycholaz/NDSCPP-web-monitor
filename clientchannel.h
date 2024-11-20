#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <chrono>
#include "interfaces.h"
#include "utilities.h"
#include "pixeltypes.h"

class ClientChannel : public ISocketChannel
{
public:
    ClientChannel(const std::string& hostName,
                  const std::string& friendlyName,
                  uint32_t width,
                  uint32_t height = 1,
                  uint32_t offset = 0,
                  bool compressData = true,
                  uint8_t channel = 0,
                  uint32_t batchSize = 1,
                  bool redGreenSwap = false)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _compressData(compressData),
          _width(width),
          _height(height),
          _offset(offset),
          _channel(channel),
          _batchSize(batchSize),
          _redGreenSwap(redGreenSwap),
          _running(false) {}

    ~ClientChannel() override { Stop(); }

    void Start() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running)
        {
            _running = true;
            _worker = std::thread(&ClientChannel::WorkerLoop, this);
        }
    }

    void Stop() override
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_running)
                return;
            _running = false;
        }

        _condition.notify_all();
        if (_worker.joinable())
            _worker.join();
    }

    virtual bool EnqueueFrame(const std::vector<uint8_t>& frameData, std::chrono::time_point<std::chrono::system_clock> timestamp) override
    {
        std::lock_guard<std::mutex> lock(_queueMutex);

        if (_queue.size() >= MaxQueueDepth)
            _queue.pop(); // Discard the oldest frame

        _queue.push(frameData);
        _condition.notify_one();
        return true;
    }

private:
    std::vector<uint8_t> GetFrameData(const std::vector<CRGB>& leds) const
    {
        auto pixelData = Utilities::GetColorBytesAtOffset(leds, _offset, _width * _height, false, _redGreenSwap);

        // Calculate epoch time
        auto now = std::chrono::system_clock::now();
        auto epoch = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        uint64_t seconds = epoch / 1'000'000;
        uint64_t microseconds = epoch % 1'000'000;

        return Utilities::CombineByteArrays(
            {
                Utilities::WORDToBytes(CommandPixelData),
                Utilities::WORDToBytes(_channel),
                Utilities::DWORDToBytes(static_cast<uint32_t>(pixelData.size() / 3)),
                Utilities::ULONGToBytes(seconds),
                Utilities::ULONGToBytes(microseconds),
                pixelData
            }
        );
    }

    void WorkerLoop()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            _condition.wait(lock, [this] { return !_queue.empty() || !_running; });

            if (!_running && _queue.empty())
                return;

            auto data = _queue.front();
            _queue.pop();
            lock.unlock();

            // Send data over the network (placeholder for actual implementation)
            SendData(data);
        }
    }

    void SendData(const std::vector<uint8_t>& data)
    {
        // TODO: Implement the actual socket communication logic
    }

private:
    static constexpr uint16_t CommandPixelData = 3;
    static constexpr size_t MaxQueueDepth = 100;

    std::string _hostName;
    std::string _friendlyName;
    uint32_t _width;
    uint32_t _height;
    uint32_t _offset;
    bool _compressData;
    uint8_t _channel;
    uint32_t _batchSize;
    bool _redGreenSwap;

    mutable std::mutex _mutex;
    std::mutex _queueMutex;
    std::condition_variable _condition;
    std::queue<std::vector<uint8_t>> _queue;
    std::thread _worker;
    bool _running;
};
