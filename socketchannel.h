#pragma once
using namespace std;

#include <iostream>
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
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include "interfaces.h"
#include "utilities.h"
#include "pixeltypes.h"
#include "serialization.h"

// SocketChannel
//
// Represents a socket connection to a NightDriverStrip client. Keeps a queue of frames and 
// pops them off the queue and sends them on a worker thread. The worker thread will attempt
// to connect to the client if it is not already connected. The worker thread will also
// attempt to reconnect if the connection is lost.

class ClientResponse;

class SocketChannel : public ISocketChannel
{
public:
    SocketChannel(const string& hostName, const string& friendlyName, uint16_t port = 49152)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _port(port),
          _isConnected(false),
          _running(false),
          _socketFd(-1),
          _lastClientResponse(),
          _dataSentCount(0),
          _lastDataSentCountReset(system_clock::now()),
          _lastConnectionAttempt(system_clock::now())
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
    
    const string& HostName() const override { return _hostName; }
    const string& FriendlyName() const override { return _friendlyName; }
    
    const ClientResponse& LastClientResponse() const override 
    { 
        lock_guard<mutex> lock(_responseMutex);
        return _lastClientResponse; 
    }

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
            std::move(compressedData)
        );
    }

    bool EnqueueFrame(vector<uint8_t>&& frameData) override
    {
        lock_guard<mutex> lock(_queueMutex);
        if (_frameQueue.size() >= MaxQueueDepth)
        {
            cout << "Queue is full, dropping frame and resetting socket" << endl;
            CloseSocket();
            return false;
        }
        _frameQueue.push(std::move(frameData));
        return true;
    }

private:
    void WorkerLoop()
    {
        steady_clock::time_point lastSendTime = steady_clock::now();
        constexpr auto kMaxBatchSize = 4;
        constexpr auto xMaxBatchDelay_ms = 1000;
        constexpr auto reconnectDelay_ms = 5000; // 5 second delay between connection attempts

        while (_running)
        {
            try
            {
                vector<uint8_t> combinedBuffer;
                size_t totalBytes = 0;
                size_t packetCount = 0;

                auto now = steady_clock::now();
                auto timeToSend = duration_cast<milliseconds>(now - lastSendTime).count() >= xMaxBatchDelay_ms;

                // Only start draining if it's time to send or the queue has reached the max size
                if (!_frameQueue.empty() && (_frameQueue.size() >= kMaxBatchSize || timeToSend))
                {
                    unique_lock<mutex> lock(_queueMutex);
                    
                    while (!_frameQueue.empty() && packetCount < kMaxBatchSize)
                    {
                        vector<uint8_t>& frame = _frameQueue.front();
                        totalBytes += frame.size();
                        packetCount++;
                        combinedBuffer.insert(combinedBuffer.end(), frame.begin(), frame.end());
                        _frameQueue.pop();
                    }

                    lastSendTime = steady_clock::now();
                }

                if (!combinedBuffer.empty())
                {
                    optional<ClientResponse> response = SendFrame(std::move(combinedBuffer));
                    if (response)
                    {
                        lock_guard<mutex> lock(_responseMutex);
                        _lastClientResponse = std::move(*response);
                    }
                }
            }
            catch (const exception& e)
            {
                cerr << "WorkerLoop exception: " << e.what() << endl;
                CloseSocket();
                
                // Wait before attempting to reconnect
                auto now = system_clock::now();
                if (duration_cast<milliseconds>(now - _lastConnectionAttempt).count() < reconnectDelay_ms)
                {
                    this_thread::sleep_for(milliseconds(100));
                    continue;
                }
            }

            this_thread::sleep_for(milliseconds(1));
        }
    }

    optional<ClientResponse> ReadSocketResponse() 
    {
        const size_t cbToRead = sizeof(ClientResponse);
        vector<uint8_t> buffer(cbToRead);

        pollfd pfd;
        pfd.fd = _socketFd;
        pfd.events = POLLIN;

        if (poll(&pfd, 1, 0) <= 0) 
            return nullopt;

        ssize_t readBytes = recv(_socketFd, buffer.data(), cbToRead, 0);
        if (readBytes == static_cast<ssize_t>(cbToRead)) 
        {
            if (buffer[0] != static_cast<uint8_t>(cbToRead))
            {
                cerr << "Invalid response size received from client" << endl;
                return nullopt;
            }

            // Validate the response before copying
            if (!ValidateClientResponse(buffer.data(), cbToRead))
            {
                cerr << "Invalid client response format" << endl;
                return nullopt;
            }

            ClientResponse response;
            memcpy(&response, buffer.data(), cbToRead);
            response.TranslateClientResponse();

            cout << response.currentClock - system_clock::to_time_t(system_clock::now()) 
                 << " seconds behind" << endl;

            return response;
        }

        return nullopt;
    }

    bool ValidateClientResponse(const void* data, size_t size) const
    {
        if (size != sizeof(ClientResponse))
            return false;

        const ClientResponse* response = static_cast<const ClientResponse*>(data);
        
        // Add validation logic based on your ClientResponse structure
        // For example, check if fields are within expected ranges
        
        return true; // Implement actual validation
    }

    bool SetSocketNonBlocking(int socketFd)
    {
        int flags = fcntl(socketFd, F_GETFL, 0);
        if (flags == -1) return false;
        return (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) != -1);
    }

    optional<ClientResponse> SendFrame(const vector<uint8_t>&& frame)
    {
        if (_socketFd == -1)
        {
            if (!ConnectSocket())
            {
                lock_guard<mutex> lock(_mutex);
                _isConnected = false;
                return nullopt;
            }
        }

        size_t totalSent = 0;
        while (totalSent < frame.size())
        {
            ssize_t sent = send(_socketFd, 
                               frame.data() + totalSent, 
                               frame.size() - totalSent, 
                               MSG_NOSIGNAL | MSG_DONTWAIT);
            
            if (sent == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // Socket buffer full, wait and retry
                    this_thread::sleep_for(milliseconds(1));
                    continue;
                }
                CloseSocket();
                lock_guard<mutex> lock(_mutex);
                _isConnected = false;
                return nullopt;
            }
            totalSent += sent;
        }

        {
            lock_guard<mutex> lock(_mutex);
            _isConnected = true;
            _dataSentCount += totalSent;
        }

        return ReadSocketResponse();
    }

    bool ConnectSocket()
    {
        _lastConnectionAttempt = system_clock::now();
        
        int tempSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (tempSocket == -1)
            return false;

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(_port);

        if (inet_pton(AF_INET, _hostName.c_str(), &serverAddr.sin_addr) <= 0)
        {
            close(tempSocket);
            return false;
        }

        // Set socket to non-blocking mode
        if (!SetSocketNonBlocking(tempSocket))
        {
            close(tempSocket);
            return false;
        }

        // Non-blocking connect
        int result = connect(tempSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result == -1)
        {
            if (errno != EINPROGRESS)
            {
                close(tempSocket);
                return false;
            }

            // Wait for connection with timeout
            pollfd pfd;
            pfd.fd = tempSocket;
            pfd.events = POLLOUT;
            
            if (poll(&pfd, 1, 5000) <= 0) // 5 second timeout
            {
                close(tempSocket);
                return false;
            }

            // Check if connection was successful
            int error = 0;
            socklen_t len = sizeof(error);
            if (getsockopt(tempSocket, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0)
            {
                close(tempSocket);
                return false;
            }
        }

        cout << "Connected to " << _hostName << ":" << _port << endl;
        _socketFd = tempSocket;
        return true;
    }

    void CloseSocket()
    {
        lock_guard<mutex> lock(_mutex);
        cout << "Closing socket connection to " << _hostName << endl;
        
        // Clear the queue
        {
            lock_guard<mutex> queueLock(_queueMutex);
            queue<vector<uint8_t>> empty;
            _frameQueue.swap(empty);
        }

        if (_socketFd != -1)
        {
            close(_socketFd);
            _socketFd = -1;
        }
        _isConnected = false;
    }

    uint32_t BytesPerSecond()
    {
        lock_guard<mutex> lock(_mutex);
        
        system_clock::time_point now = system_clock::now();
        auto elapsed = duration_cast<duration<double>>(now - _lastDataSentCountReset).count();

        if (elapsed == 0.0)
            return 0;

        uint32_t bytesPerSecond = static_cast<uint32_t>(_dataSentCount / elapsed);

        constexpr auto bpsResetTimer = 3.0;
        if (elapsed >= bpsResetTimer)
        {
            _lastDataSentCountReset = now;
            _dataSentCount = 0;
        }

        return bytesPerSecond;
    }

private:
    static constexpr uint16_t CommandPixelData = 3;
    static constexpr size_t MaxQueueDepth = 100;

    string _hostName;
    string _friendlyName;
    uint16_t _port;

    mutable mutex _mutex;
    mutable mutex _queueMutex;
    mutable mutex _responseMutex;
    
    atomic<bool> _isConnected;
    atomic<bool> _running;

    queue<vector<uint8_t>> _frameQueue;
    thread _workerThread;

    ClientResponse _lastClientResponse;
    system_clock::time_point _lastDataSentCountReset;
    system_clock::time_point _lastConnectionAttempt;
    uint32_t _dataSentCount;
    
    int _socketFd;
};