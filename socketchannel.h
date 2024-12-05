#pragma once
using namespace std;

#include <iostream>
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
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <cerrno>
#include "interfaces.h"
#include "clientresponse.h"
#include "utilities.h"
#include "pixeltypes.h"

// How long to wait for a connection to be established or data sent

constexpr auto kConnectTimeout = 3000ms; 
constexpr auto kSendTimeout    = 2000ms;

// SpeedTracker
// 
// A class that tracks the speed of data transfer over a given time window.  It is used to
// calculate the bytes per second that are being sent to the client.  The class uses a
// weighted average to smooth out the data and provide a more accurate representation of
// the speed.

class SpeedTracker
{
private:
    static constexpr milliseconds kSpeedWindowMS{3000}; // 3 second window
    static constexpr double kPreviousWindowWeight{0.3}; // Weight for previous window in average

    uint64_t _currentWindowBytes{0};
    uint64_t _previousWindowBytes{0};
    system_clock::time_point _windowStartTime;

public:
    SpeedTracker() : _windowStartTime(system_clock::now()) {}

    void AddBytes(uint64_t bytes)
    {
        // Check for overflow before adding.  No idea if overflow is a practical
        // concern, but I'd feel weird not checking.

        if (_currentWindowBytes <= (numeric_limits<uint64_t>::max() - bytes))
            _currentWindowBytes += bytes;
    }

    uint64_t UpdateBytesPerSecond()
    {
        auto now = system_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - _windowStartTime);

        // If we haven't completed a window yet, calculate based on partial window
        if (elapsed < kSpeedWindowMS)
        {
            if (elapsed.count() == 0)
                return 0; // Avoid division by zero

            // Scale up partial window to full second
            double currentRate = (_currentWindowBytes * 1000.0) / elapsed.count();

            // Blend with previous window data
            double previousRate = (_previousWindowBytes * 1000.0) / kSpeedWindowMS.count();
            return static_cast<uint64_t>(
                (currentRate * (1.0 - kPreviousWindowWeight)) +
                (previousRate * kPreviousWindowWeight));
        }

        // Window complete - rotate windows
        _previousWindowBytes = _currentWindowBytes;
        _currentWindowBytes = 0;
        _windowStartTime = now;

        // Calculate blended rate
        double previousRate = (_previousWindowBytes * 1000.0) / kSpeedWindowMS.count();
        return static_cast<uint64_t>(previousRate);
    }

    uint64_t GetLastBytesPerSecond() const
    {
        return (_previousWindowBytes * 1000) / kSpeedWindowMS.count();
    }
};

// SocketChannel
//
// Represents a socket connection to a NightDriverStrip client. Keeps a queue of frames and 
// pops them off the queue and sends them on a worker thread. The worker thread will attempt
// to connect to the client if it is not already connected. The worker thread will also
// attempt to reconnect if the connection is lost.

struct ClientResponse;
class SocketChannel : public ISocketChannel
{
    static constexpr uint16_t CommandPixelData = 3;
    static constexpr size_t MaxQueueDepth = 500;
    static constexpr size_t MaxQueuedBytes = 1024 * 1024 * 10;  // 10MB memory limit

    string _hostName;
    string _friendlyName;
    uint16_t _port;

    static atomic<uint32_t> _nextId;
    uint32_t _id;

    mutable mutex _mutex;                       // 
    mutable mutex _queueMutex;
    mutable mutex _responseMutex;
    
    atomic<bool> _isConnected;
    atomic<bool> _running;

    int _socketFd;

    ClientResponse _lastClientResponse;
    system_clock::time_point _lastResponseTime;
    system_clock::time_point _lastConnectionAttempt;
    SpeedTracker _speedTracker;

    uint32_t _reconnectCount;

    queue<vector<uint8_t>> _frameQueue;
    size_t _totalQueuedBytes;  // Track total memory usage
    thread _workerThread;


public:
    SocketChannel(const string& hostName, const string& friendlyName, uint16_t port = 49152)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _port(port),
          _id(_nextId++),
          _isConnected(false),
          _running(false),
          _socketFd(-1),
          _lastClientResponse(),
          _lastConnectionAttempt(system_clock::now()),
          _reconnectCount(0),
          _totalQueuedBytes(0)
    {
    }

    ~SocketChannel() override
    {
        Stop();
        CloseSocket();
    }

    uint32_t Id() const override 
    { 
        return _id; 
    }

    size_t GetCurrentQueueDepth() const override
    {
        lock_guard lock(_queueMutex);
        return _frameQueue.size();  
    }

    size_t GetQueueMaxSize() const override
    {
        return MaxQueueDepth;
    }

    uint32_t GetReconnectCount() const override
    {
        lock_guard lock(_mutex);
        return _reconnectCount;
    }

    virtual uint64_t GetLastBytesPerSecond() const override
    {
        return _speedTracker.GetLastBytesPerSecond();
    }

    uint16_t Port() const override
    {
        return _port;
    }

    void Start() override
    {
        lock_guard lock(_mutex);
        if (!_running)
        {
            _running = true;
            _workerThread = thread(&SocketChannel::WorkerLoop, this);
        }
    }

    void Stop() override
    {
        {
            lock_guard lock(_mutex);
            _running = false;
        }

        if (_workerThread.joinable())
            _workerThread.join();

        CloseSocket();
    }

    bool IsConnected() const override
    {
        lock_guard lock(_mutex);
        return _isConnected;
    }
    
    const string& HostName() const override { return _hostName; }
    const string& FriendlyName() const override { return _friendlyName; }
    
    // LastClientResponse
    // 
    // A copy of the last success/stats packet we got back from the client
    
    ClientResponse LastClientResponse() const override  // Changed to return by value
    { 
        constexpr auto kMaxResponseAge = 2s;

        lock_guard lock(_responseMutex);
        if (_lastResponseTime - system_clock::now() > kMaxResponseAge)
            return ClientResponse {}; // Return empty response if too old

        return _lastClientResponse; 
    }

    // CompressFrame
    //
    // Takes a frame of binary data, compresses it, and inserts a small header
    // in front of it with a magic number and the size of the compressed data.

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
    bool isQueueFull = false;
    {
        lock_guard lock(_queueMutex);
        size_t newTotalBytes = _totalQueuedBytes + frameData.size();
        if (_frameQueue.size() >= MaxQueueDepth || newTotalBytes > MaxQueuedBytes)
            isQueueFull = true;
        else {
            _totalQueuedBytes += frameData.size();
            _frameQueue.push(std::move(frameData));
        }
    }

    // If the queue is full, we reset the socket and drop the frames in the queue

    if (isQueueFull)
    {
        cout << "Queue is full at " << _hostName << " [" << _friendlyName << "] dropping frame and resetting socket" << endl;
        CloseSocket();
        EmptyQueue();
        return false;
    }

    return true;
}

private:

    // Worker Loop
    //
    // The main duties of the WorkerLoop are to send frames to the client and read responses from the client.
    // It continually watches for new packets to appear int he queue and then sends them in batches.
    // It also reads responses from the client and updates the lastClientResponse member variable.

    void WorkerLoop()
    {
        steady_clock::time_point lastSendTime = steady_clock::now();
        constexpr auto kMaxBatchSize = 20;
        constexpr auto kMaxBatchDelay = 1000ms;  // Fixed variable name
        constexpr auto reconnectDelay = 1000ms;

        while (_running)
        {
            try
            {
                vector<uint8_t> combinedBuffer;
                size_t packetCount = 0;

                auto now = steady_clock::now();
                auto bTimeToSend = duration_cast<milliseconds>(now - lastSendTime) >= kMaxBatchDelay;

                // Calculate total bytes first to preallocate buffer
                {
                    unique_lock<mutex> lock(_queueMutex);
                    size_t tempCount = 0;
                    size_t tempBytes = 0;
                    auto queueCopy = _frameQueue;
                    while (!queueCopy.empty() && tempCount < kMaxBatchSize)
                    {
                        tempBytes += queueCopy.front().size();
                        tempCount++;
                        queueCopy.pop();
                    }
                    if (tempBytes > 0) {
                        combinedBuffer.reserve(tempBytes);
                    }
                }

                if (!_frameQueue.empty() && (_frameQueue.size() >= kMaxBatchSize || bTimeToSend))
                {
                    unique_lock<mutex> lock(_queueMutex);
                    
                    while (!_frameQueue.empty() && packetCount < kMaxBatchSize)
                    {
                        vector<uint8_t>& frame = _frameQueue.front();
                        packetCount++;
                        combinedBuffer.insert(combinedBuffer.end(), frame.begin(), frame.end());
                        _totalQueuedBytes -= frame.size();
                        _frameQueue.pop();
                    }
                }

                if (packetCount > 0)
                {
                    // cout << "Sending " << packetCount << " packets totalling " << totalBytes << " bytes" << " at queue size " << _frameQueue.size() << " to " << _hostName << endl;

                    if (!combinedBuffer.empty())
                    {
                        lastSendTime = steady_clock::now();
                        optional<ClientResponse> response = SendFrame(std::move(combinedBuffer));
                        if (response)
                        {
                            lock_guard lock(_responseMutex);
                            _lastClientResponse = std::move(*response);
                            _lastResponseTime = system_clock::now();
                        }
                        _speedTracker.UpdateBytesPerSecond();
                    }
                }
            }
            catch (const exception& e)
            {
                cerr << "WorkerLoop exception: " << e.what() << endl;
                CloseSocket();
                
                // Wait before attempting to reconnect
                auto now = system_clock::now();
                if (duration_cast<milliseconds>(now - _lastConnectionAttempt) < reconnectDelay)
                {
                    cout << "Waiting for " << duration_cast<milliseconds>(reconnectDelay - (now - _lastConnectionAttempt)).count() << "ms before reconnecting" << endl; 
                    this_thread::sleep_for(reconnectDelay - (now - _lastConnectionAttempt));
                    continue;
                }
            }

            this_thread::sleep_for(milliseconds(1));
        }
    }

    optional<ClientResponse> ReadSocketResponse() 
    {
        const size_t cbToRead = sizeof(ClientResponse);
        optional<ClientResponse> lastResponse;

        pollfd pfd;
        pfd.fd = _socketFd;
        pfd.events = POLLIN;

        // Keep reading while there's data available
        while (poll(&pfd, 1, 0) > 0)
        {
            uint8_t byteCount = 0;
            ssize_t readBytes = recv(_socketFd, &byteCount, 1, MSG_PEEK);
            if (readBytes <= 0)
                break;

            if (byteCount != static_cast<uint8_t>(cbToRead))
            {
                if (byteCount == sizeof(OldClientResponse))
                {
                    OldClientResponse oldResponse;
                    readBytes = recv(_socketFd, &oldResponse, sizeof(OldClientResponse), 0);
                    if (readBytes == sizeof(OldClientResponse))
                    {
                        ClientResponse response;
                        response = oldResponse;
                        response.TranslateClientResponse();
                        lastResponse = response;
                        continue;  // Check for more data
                    }
                }

                // Invalid byte count; eat the contents
                vector<uint8_t> tempBuffer(byteCount);
                recv(_socketFd, tempBuffer.data(), byteCount, 0);
                continue;  // Check for more data
            }

            // Read the full response
            vector<uint8_t> buffer(cbToRead);
            readBytes = recv(_socketFd, buffer.data(), cbToRead, 0);
            if (readBytes == static_cast<ssize_t>(cbToRead))
            {
                ClientResponse response;
                memcpy(&response, buffer.data(), cbToRead);
                response.TranslateClientResponse();
                lastResponse = response;
                continue;  // Check for more data
            }

            break;  // Error reading data
        }

        return lastResponse;
    }

    bool SetSocketOptions(int socketFd)
    {
        // Set socket to non-blocking mode

        int flags = fcntl(socketFd, F_GETFL, 0);
        if (flags == -1) 
            return false;
        
        if (!(fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) != -1))
            return false;

        // Enable TCP keepalive on the socket

        int keepalive = 1;
        int keepcnt = 3;          // Number of keepalive probes before declaring dead
        int keepidle = 1;         // Time in seconds before sending keepalive probes
        int keepintvl = 1;        // Time in seconds between keepalive probes

        // On macOS, TCP_KEEPIDLE is called TCP_KEEPALIVE
        #ifdef __APPLE__
            #define TCP_KEEPIDLE TCP_KEEPALIVE
        #endif

        if (setsockopt(socketFd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive)) < 0 ||
            setsockopt(socketFd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt)) < 0 ||
            setsockopt(socketFd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle)) < 0 ||
            setsockopt(socketFd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl)) < 0)
        {
            cerr << "Could not set keepalive options for " << _friendlyName << endl;
            return false;
        }           

        struct timeval timeouttv;
        timeouttv.tv_sec = kSendTimeout.count() / 1000;
        timeouttv.tv_usec = (kSendTimeout.count() % 1000) * 1000;

        if (setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeouttv, sizeof(timeouttv)) < 0)
        {
            cerr << "Could not set TCP send timeout for " << _friendlyName << endl;
            return false;
        }

        return true;
    }

    optional<ClientResponse> SendFrame(const vector<uint8_t>&& frame)
    {
        if (_socketFd == -1 && !ConnectSocket())
        {
            lock_guard lock(_mutex);
            _isConnected = false;
            return nullopt;
        }

        size_t totalSent = 0;
        while (totalSent < frame.size() && _running)
        {
            auto startTime = steady_clock::now();

            ssize_t sent = send(_socketFd, 
                            frame.data() + totalSent, 
                            frame.size() - totalSent, 
                            MSG_NOSIGNAL);
            
            if (sent > 0)
            {
                totalSent += sent;
                continue;
            }

            if (sent == -1)
            {
                if (errno == EPIPE)
                {
                    CloseSocket();
                    if (!ConnectSocket()) 
                        return nullopt;
                    continue;
                }
                
                if ((errno == EWOULDBLOCK || errno == EAGAIN) && ((steady_clock::now() - startTime) < kSendTimeout))
                {
                    this_thread::sleep_for(100ms);
                    continue;
                }
                cerr << "Socket timed out for " << _hostName << " [" << _friendlyName << "] errno=" << errno << endl;

                CloseSocket();
                return nullopt;
            }
        }

        {
            lock_guard lock(_mutex);
            _isConnected = true;
            _speedTracker.AddBytes(totalSent);
        }

        return _running ? ReadSocketResponse() : nullopt;
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

        // Set socket options (non-blocking, keepalive, send timeout)
        if (!SetSocketOptions(tempSocket))
        {
            cerr << "Could not set socket options for " << _friendlyName << endl;
            close(tempSocket);
            return false;
        }

        // Non-blocking connect
        int result = connect(tempSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        if (result == -1)
        {
            if (errno != EINPROGRESS)
            {
                cerr << "Could not connect to " << _hostName << " [" << _friendlyName << "] errno=" << errno << endl;
                close(tempSocket);
                return false;
            }

            // Wait for connection with timeout
            pollfd pfd;
            pfd.fd = tempSocket;
            pfd.events = POLLOUT;
            
            if (poll(&pfd, 1, kConnectTimeout.count()) <= 0)
            {
                cerr << "Connection timeout to " << _hostName << " [" << _friendlyName << "]" << endl;
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

        _reconnectCount++;
        cout << "Connection number " << _reconnectCount << " to " << _hostName << ":" << _port << " [" << _friendlyName << "] on thread " << this_thread::get_id() << endl; 
        _socketFd = tempSocket;
        return true;
    }

    void EmptyQueue()
    {
        scoped_lock lock(_mutex, _queueMutex);
        queue<vector<uint8_t>> empty;
        _frameQueue.swap(empty);
        _totalQueuedBytes = 0;
    }

    void CloseSocket()
    {
        lock_guard lock(_mutex);  // Add lock    
        if (_socketFd != -1)
        {
            close(_socketFd);
            _socketFd = -1;
        }
        _isConnected = false;
    }
};


inline void from_json(const nlohmann::json& j, unique_ptr<ISocketChannel>& socket) 
{
    socket = make_unique<SocketChannel>(
        j.at("hostName").get<string>(),
        j.at("friendlyName").get<string>(),
        j.value("port", uint16_t(49152))
    );
}
