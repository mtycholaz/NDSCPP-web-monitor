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
#include "interfaces.h"
#include "utilities.h"
#include "pixeltypes.h"
#include "serialization.h"

// SocketChannel
//
// Represents a socket connection to a NightDriverStrip client.  Keeps a queue of frames and 
// pops them off the queue and sends them on a worker thread.  The worker thread will attempt
// to connect to the client if it is not already connected.  The worker thread will also
// attempt to reconnect if the connection is lost.

class ClientResponse;

class SocketChannel : public ISocketChannel
{
public:
    SocketChannel(const string & hostName, const string & friendlyName, uint16_t port = 49152)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _port(port),
          _isConnected(false),
          _running(false),
          _socketFd(-1),
          _lastClientResponse()                 // Zero-initialize the response
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
    
    const string & HostName()     const override { return _hostName;     }
    const string & FriendlyName() const override { return _friendlyName; }
    
    const ClientResponse & LastClientResponse() const override { return _lastClientResponse; }

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

    bool EnqueueFrame(vector<uint8_t> && frameData) override
    {
        cout << "Adding Frame to Queue at queue depth " << _frameQueue.size() << endl;
        {
            lock_guard<mutex> lock(_queueMutex);
            if (_frameQueue.size() >= MaxQueueDepth)
                return false; // Queue is full
            _frameQueue.push(std::move(frameData));
        }
        _queueCondition.notify_one(); // Notify the worker thread that a new frame is available
        return true; // Successfully enqueued
    }

private:

    void WorkerLoop()
    {
        while (_running)
        {
            vector<uint8_t> combinedBuffer; // Buffer to hold all combined frames
            size_t totalBytes = 0;
            size_t packetCount = 0;
            constexpr auto kMaxBatchSize = 20;
            {
                unique_lock<mutex> lock(_queueMutex);

                // Drain the queue into the combined buffer
                while (!_frameQueue.empty() && packetCount < kMaxBatchSize)
                {
                    vector<uint8_t>& frame = _frameQueue.front();
                    totalBytes += frame.size();
                    packetCount++;
                    combinedBuffer.insert(combinedBuffer.end(), frame.begin(), frame.end());
                    _frameQueue.pop();
                }
            }

            // If we have data to send, send the combined buffer
            if (!combinedBuffer.empty())
            {
                cout << "Sending Combined Frame " << system_clock::now() 
                     << " (" << totalBytes << " bytes, " << packetCount << " packets)" 
                     << endl;

                optional<ClientResponse> response = SendFrame(std::move(combinedBuffer));
                if (response)
                    _lastClientResponse = std::move(*response);
            }

            // Sleep briefly to yield to other threads
            this_thread::sleep_for(milliseconds(1));
        }
    }


    optional<ClientResponse> ReadSocketResponse() 
    {
        const size_t cbToRead = sizeof(ClientResponse);
        vector<uint8_t> buffer(cbToRead); // Buffer to hold the response

        // Check if data is available

      #ifdef _WIN32

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(_socketFd, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 0; // Non-blocking

        if (select(0, &readSet, nullptr, nullptr, &timeout) <= 0) 
            return nullopt; // No data available

      #else

        pollfd pfd;
        pfd.fd = _socketFd;
        pfd.events = POLLIN;

        if (poll(&pfd, 1, 0) <= 0) 
            return nullopt; // No data available

      #endif

        // Read data if available and is at least as big as our response structure.  A partial
        // read is not considered a valid response, so will be skipped until we have a full response.
        // The first byte of the response is the size of the response, so we can use that to determine
        // if we are compatible with the client's response.

        ssize_t readBytes = recv(_socketFd, buffer.data(), cbToRead, 0);
        if (readBytes == static_cast<ssize_t>(cbToRead)) 
        {
            if (buffer[0] != static_cast<uint8_t>(cbToRead))
            {
                cerr << "Invalid response size received from client" << endl;
                return nullopt; // Invalid response
            }
            ClientResponse response;
            memcpy(&response, buffer.data(), cbToRead);
            response.TranslateClientResponse(); // Translate the response to native endianness

            cout << response.currentClock - system_clock::to_time_t(system_clock::now()) << " seconds behind" << endl;

            return response; // Successfully read the response
        }

        return nullopt; // Not enough data or invalid response
    }

    // SendFrame
    //
    // Delivers a bytestream packet to the client socket and returns a response from that client if 
    // available.  
     
    optional<ClientResponse> SendFrame(const vector<uint8_t> && frame)
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

        try
        {
            ssize_t sentBytes = send(_socketFd, frame.data(), frame.size(), MSG_NOSIGNAL);
            if (sentBytes == -1)
            {
                CloseSocket();
                lock_guard<mutex> lock(_mutex);
                _isConnected = false;
                return nullopt;
            }
            _dataSentCount += sentBytes;
        }
        catch(const exception& e)
        {

            cerr << e.what() << '\n';
            CloseSocket();
            lock_guard<mutex> lock(_mutex);
            _isConnected = false;
        }
        
        {
            lock_guard<mutex> lock(_mutex);
            _isConnected = true;
        }

        return std::nullopt;

        optional<ClientResponse> response = ReadSocketResponse();
        if (!response)
            return std::nullopt;
            
        return std::move(*response);
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

        cout << "Connecting to " << _hostName << ":" << _port << endl;
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

    uint32_t BytesPerSecond()
    {
        system_clock::time_point now = system_clock::now();
        auto elapsed = duration_cast<duration<double>>(now - _lastDataSentCountReset).count(); // Elapsed time in fractional seconds

        if (elapsed == 0.0)
            return 0;

        uint32_t bytesPerSecond = static_cast<uint32_t>(_dataSentCount / elapsed); // Calculate accurate BPS
        _dataSentCount = 0;
        _lastDataSentCountReset = now;

        // Reset the BPS counter every three seconds
        constexpr auto bpsResetTimer = 3.0; 
        if (elapsed >= bpsResetTimer)
        {
            _lastDataSentCountReset = now;
            _dataSentCount = 0;
        }

        return bytesPerSecond;
    }

    bool SocketConnected()
    {
        return _isConnected;
    }

    bool SocketReady()
    {
        if (_socketFd == -1)
            return false;

        // Check if the socket is still connected
        struct pollfd pfd;
        pfd.fd = _socketFd;
        pfd.events = POLLIN;

        if (poll(&pfd, 1, 0) <= 0)
            return false; // Not connected

        return true;
    }
    
private:
    // Constants
    static constexpr uint16_t CommandPixelData = 3;
    static constexpr size_t   MaxQueueDepth = 100;

    // Member variables
    string   _hostName;
    string   _friendlyName;
    uint16_t _port;

    mutable mutex _mutex; // Protects connection state
    atomic<bool>  _isConnected;
    atomic<bool>  _running;

    mutex _queueMutex; // Protects the frame queue
    queue<vector<uint8_t>> _frameQueue;
    condition_variable _queueCondition; // Condition variable to signal the worker thread


    thread _workerThread;

    ClientResponse _lastClientResponse;

    system_clock::time_point _lastDataSentCountReset;
    uint32_t                 _dataSentCount;
    
    int _socketFd; // File descriptor for the socket
};
