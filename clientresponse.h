#pragma once
using namespace std;

// ClientResponse
//
// Response data sent back to server every time we receive a packet.
// This struct is packed to match the exact network protocol format used by ESP32 clients.
// The packed attribute is required to ensure correct network communication but may cause
// alignment issues on some architectures.

#include "json.hpp"
#include <cstdint>
#include <cstring>
#include <bit>

inline static double ByteSwapDouble(double value)
{
    // Helper function to swap bytes in a double
    uint64_t temp;
    memcpy(&temp, &value, sizeof(double)); // Copy bits of double to temp
    temp = __builtin_bswap64(temp);        // Byte swap the 64-bit integer
    memcpy(&value, &temp, sizeof(double)); // Copy bits back to double
    return value;
}

struct OldClientResponse
{
    uint32_t size;         // 4
    uint32_t flashVersion; // 4
    double currentClock;   // 8
    double oldestPacket;   // 8
    double newestPacket;   // 8
    double brightness;     // 8
    double wifiSignal;     // 8
    uint32_t bufferSize;   // 4
    uint32_t bufferPos;    // 4
    uint32_t fpsDrawing;   // 4
    uint32_t watts;        // 4
} __attribute__((packed)); // Packed attribute required for network protocol compatibility

struct ClientResponse
{
    uint32_t size = sizeof(ClientResponse);         // 4
    uint64_t sequence = 0;                          // 8
    uint32_t flashVersion = 0;                      // 4
    double currentClock = 0;                        // 8
    double oldestPacket = 0;                        // 8
    double newestPacket = 0;                        // 8
    double brightness = 0;                          // 8
    double wifiSignal = 0;                          // 8
    uint32_t bufferSize = 0;                        // 4
    uint32_t bufferPos = 0;                         // 4
    uint32_t fpsDrawing = 0;                        // 4
    uint32_t watts = 0;                             // 4

    ClientResponse& operator=(const OldClientResponse& old)
    {
        size = sizeof(ClientResponse);;
        sequence = 0;  // New field, initialize to 0
        flashVersion = old.flashVersion;
        currentClock = old.currentClock;
        oldestPacket = old.oldestPacket;
        newestPacket = old.newestPacket;
        brightness = old.brightness;
        wifiSignal = old.wifiSignal;
        bufferSize = old.bufferSize;
        bufferPos = old.bufferPos;
        fpsDrawing = old.fpsDrawing;
        watts = old.watts;
        return *this;
    }

    // Member function to translate the structure from the ESP32 little endian
    // to whatever the current running system is

    void TranslateClientResponse()
    {
        // Check the system's endianness
        if constexpr (endian::native == endian::little)
            return; // No-op for little-endian systems

        // Perform byte swaps for big-endian systems
        size = __builtin_bswap32(size);
        sequence = __builtin_bswap64(sequence); // Added missing sequence swap
        flashVersion = __builtin_bswap32(flashVersion);
        currentClock = ByteSwapDouble(currentClock);
        oldestPacket = ByteSwapDouble(oldestPacket);
        newestPacket = ByteSwapDouble(newestPacket);
        brightness = ByteSwapDouble(brightness);
        wifiSignal = ByteSwapDouble(wifiSignal);
        bufferSize = __builtin_bswap32(bufferSize);
        bufferPos = __builtin_bswap32(bufferPos);
        fpsDrawing = __builtin_bswap32(fpsDrawing);
        watts = __builtin_bswap32(watts);
    }
} __attribute__((packed)); // Packed attribute required for network protocol compatibility

// One could argue this should be in the "serialization.h" file, but that creates a 
// dependency problem in socketchannel.h. The serialization code being this close to the
// structure definitions is defendable, in my view.

inline void to_json(nlohmann::json &j, const ClientResponse &response)
{
    j ={
            {"responseSize", response.size},
            {"sequenceNumber", response.sequence},
            {"flashVersion", response.flashVersion},
            {"currentClock", response.currentClock},
            {"oldestPacket", response.oldestPacket},
            {"newestPacket", response.newestPacket},
            {"brightness", response.brightness},
            {"wifiSignal", response.wifiSignal},
            {"bufferSize", response.bufferSize},
            {"bufferPos", response.bufferPos},
            {"fpsDrawing", response.fpsDrawing},
            {"watts", response.watts}
    };
}

inline void from_json(const nlohmann::json& j, ClientResponse& response) 
{
    response.size = j.at("responseSize").get<uint8_t>();
    response.sequence = j.at("sequenceNumber").get<uint32_t>();
    response.flashVersion = j.at("flashVersion").get<uint32_t>();
    response.currentClock = j.at("currentClock").get<uint64_t>();
    response.oldestPacket = j.at("oldestPacket").get<uint64_t>();
    response.newestPacket = j.at("newestPacket").get<uint64_t>();
    response.brightness = j.at("brightness").get<uint8_t>();
    response.wifiSignal = j.at("wifiSignal").get<int8_t>();
    response.bufferSize = j.at("bufferSize").get<uint32_t>();
    response.bufferPos = j.at("bufferPos").get<uint32_t>();
    response.fpsDrawing = j.at("fpsDrawing").get<float>();
    response.watts = j.at("watts").get<float>();
}

