#pragma once
using namespace std;

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
    SocketChannel(const string &hostName, const string &friendlyName, uint16_t port = 49152)
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
        lock_guard<mutex> lock(_mutex);
        if (!_running)
        {
            _running = true;
            _workerThread = thread(&SocketChannel::WorkerLoop, this);
        }
    }

    void Stop() override
    {
        {
            lock_guard<mutex> lock(_mutex);
            _running = false;
        }

        if (_workerThread.joinable())
            _workerThread.join();

        CloseSocket();
    }

    bool IsConnected() const override
    {
        lock_guard<mutex> lock(_mutex);
        return _isConnected;
    }
    
    const string &HostName() const override { return _hostName; }
    const string &FriendlyName() const override { return _friendlyName; }

    vector<uint8_t> CompressFrame(const vector<uint8_t>& data) override
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

    bool EnqueueFrame(const vector<uint8_t> &frameData) override
    {
        {
            lock_guard<mutex> lock(_queueMutex);
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
            vector<uint8_t> frame;
            {
                lock_guard<mutex> lock(_queueMutex);
                if (!_frameQueue.empty())
                {
                    frame = _frameQueue.front();
                    _frameQueue.pop();
                }
            }

            if (!frame.empty())
                SendFrame(frame);

            this_thread::sleep_for(milliseconds(10)); // Adjust based on requirements
        }
    }

    void SendFrame(const vector<uint8_t> &frame)
    {
        if (_socketFd == -1)
        {
            if (!ConnectSocket())
            {
                lock_guard<mutex> lock(_mutex);
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
                lock_guard<mutex> lock(_mutex);
                _isConnected = false;
                return;
            }
        }
        catch(const exception& e)
        {

            cerr << e.what() << '\n';
            CloseSocket();
            lock_guard<mutex> lock(_mutex);
            _isConnected = false;
        }
        
        lock_guard<mutex> lock(_mutex);
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
    string _hostName;
    string _friendlyName;
    uint16_t _port;

    mutable mutex _mutex; // Protects connection state
    atomic<bool> _isConnected;
    atomic<bool> _running;

    mutex _queueMutex; // Protects the frame queue
    queue<vector<uint8_t>> _frameQueue;

    thread _workerThread;

    int _socketFd; // File descriptor for the socket
};
