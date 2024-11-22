#pragma once

// SocketChannel
//
// Represents a socket connection to a NightDriverStrip client.  Keeps a queue of frames and 
// pops them off the queue and sends them on a worker thread.  The worker thread will attempt
// to connect to the client if it is not already connected.  The worker thread will also
// attempt to reconnect if the connection is lost.
 
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <thread>
#include <stdexcept>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "interfaces.h"
#include "utilities.h"
#include "pixeltypes.h"

class SocketChannel : public ISocketChannel
{
public:
    SocketChannel(const std::string &hostName, const std::string &friendlyName, uint16_t port = 49152)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _port(port),
          _isConnected(false),
          _running(false),
          _socketFd(-1)
    {
    }

    ~SocketChannel() override
    {
        Stop();
        CloseSocket();
    }

    uint16_t Port() const override
    {
        return _port;
    }

    // Override methods from ISocketChannel
    void Start() override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_running)
        {
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

        CloseSocket();
    }

    bool IsConnected() const override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _isConnected;
    }
    
    const std::string &HostName() const override { return _hostName; }
    const std::string &FriendlyName() const override { return _friendlyName; }

    std::vector<uint8_t> CompressFrame(const std::vector<uint8_t>& data) override
    {
        constexpr uint32_t COMPRESSED_HEADER_TAG = 0x44415645; // Magic "DAVE" tag
        constexpr uint32_t CUSTOM_TAG = 0x12345678;

        // Compress the data
        auto compressedData = Utilities::Compress(data);

        // Create the compressed frame
        return Utilities::CombineByteArrays(
            Utilities::DWORDToBytes(COMPRESSED_HEADER_TAG),
            Utilities::DWORDToBytes(static_cast<uint32_t>(compressedData.size())),
            Utilities::DWORDToBytes(static_cast<uint32_t>(data.size())),
            Utilities::DWORDToBytes(CUSTOM_TAG),
            compressedData
        );
    }

    bool EnqueueFrame(const std::vector<uint8_t> &frameData) override
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
    void WorkerLoop()
    {
        while (_running)
        {
            std::vector<uint8_t> frame;
            {
                std::lock_guard<std::mutex> lock(_queueMutex);
                if (!_frameQueue.empty())
                {
                    frame = _frameQueue.front();
                    _frameQueue.pop();
                }
            }

            if (!frame.empty())
                SendFrame(frame);

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Adjust based on requirements
        }
    }

    void SendFrame(const std::vector<uint8_t> &frame)
    {
        if (_socketFd == -1)
        {
            if (!ConnectSocket())
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _isConnected = false;
                return;
            }
        }

        try
        {
            ssize_t sentBytes = send(_socketFd, frame.data(), frame.size(), MSG_NOSIGNAL);
            if (sentBytes == -1)
            {
                CloseSocket();
                std::lock_guard<std::mutex> lock(_mutex);
                _isConnected = false;
                return;
            }
        }
        catch(const std::exception& e)
        {

            std::cerr << e.what() << '\n';
            CloseSocket();
            std::lock_guard<std::mutex> lock(_mutex);
            _isConnected = false;
        }
        
        std::lock_guard<std::mutex> lock(_mutex);
        _isConnected = true;
    }

    bool ConnectSocket()
    {
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(_port);

        if (inet_pton(AF_INET, _hostName.c_str(), &serverAddr.sin_addr) <= 0)
            return false;

        _socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (_socketFd == -1)
            return false;

        if (connect(_socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
        {
            CloseSocket();
            return false;
        }

        return true;
    }

    void CloseSocket()
    {
        if (_socketFd != -1)
        {
            close(_socketFd);
            _socketFd = -1;
        }
    }

private:
    // Constants
    static constexpr uint16_t CommandPixelData = 3;
    static constexpr size_t MaxQueueDepth = 100;

    // Member variables
    std::string _hostName;
    std::string _friendlyName;
    uint16_t _port;

    mutable std::mutex _mutex; // Protects connection state
    std::atomic<bool> _isConnected;
    std::atomic<bool> _running;

    std::mutex _queueMutex; // Protects the frame queue
    std::queue<std::vector<uint8_t>> _frameQueue;

    std::thread _workerThread;

    int _socketFd; // File descriptor for the socket
};
