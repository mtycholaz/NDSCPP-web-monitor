#pragma once

using namespace std;
using namespace std::chrono;

// Serialization
//
// To be able to create a JSON representation of the LEDFeature object, we need to provide
// a to_json function that converts the object into a JSON object.  This function is then
// used by the nlohmann::json library to serialize the object.

#include "json.hpp"
#include "interfaces.h"
#include "palette.h"

inline static double ByteSwapDouble(double value)
{
    // Helper function to swap bytes in a double
    uint64_t temp;
    memcpy(&temp, &value, sizeof(double)); // Copy bits of double to temp
    temp = __builtin_bswap64(temp);        // Byte swap the 64-bit integer
    memcpy(&value, &temp, sizeof(double)); // Copy bits back to double
    return value;
}

// Note that in theory, for proper separation of concerns, these functions should NOT need
// access to headers that are not part of the interfaces - i.e., everything we need should
// be on an interface

// ClientResponse
//
// Response data sent back to server every time we receive a packet.
// This struct is packed to match the exact network protocol format used by ESP32 clients.
// The packed attribute is required to ensure correct network communication but may cause
// alignment issues on some architectures.

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
        if constexpr (std::endian::native == std::endian::little)
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

inline void to_json(nlohmann::json& j, const CRGB& color) 
{
    j = {
        {"r", color.r},
        {"g", color.g},
        {"b", color.b}
    };
}

inline void from_json(const nlohmann::json& j, CRGB& color) 
{
    color = CRGB(
        j.at("r").get<uint8_t>(),
        j.at("g").get<uint8_t>(),
        j.at("b").get<uint8_t>()
    );
}


// In serialization.h
inline void to_json(nlohmann::json& j, const Palette & palette) 
{
    auto colorsJson = nlohmann::json::array();
    for (const auto& color : palette.getColors()) 
        colorsJson.push_back(color);  // Uses CRGB serializer
        
    j = 
    {
        {"colors", colorsJson},
        {"blend", palette._bBlend}
    };
}

inline void from_json(const nlohmann::json& j, Palette & palette) 
{
    // Deserialize the "colors" array
    std::vector<CRGB> colors;
    for (const auto& colorJson : j.at("colors")) 
        colors.push_back(colorJson.get<CRGB>()); // Use CRGB's from_json function

    // Deserialize the "blend" flag, defaulting to true if not present
    bool blend = j.value("blend", true);

    // Assign to palette (Palette must allow reassignment)
    palette = Palette(colors, blend);
}

// Feature serialization
inline void to_json(nlohmann::json& j, const ILEDFeature & feature) 
{
    j = {
            {"type", "LEDFeature"},
            {"hostName", feature.Socket().HostName()},
            {"friendlyName", feature.Socket().FriendlyName()},
            {"port", feature.Socket().Port()},
            {"width", feature.Width()},
            {"height", feature.Height()},
            {"offsetX", feature.OffsetX()},
            {"offsetY", feature.OffsetY()},
            {"reversed", feature.Reversed()},
            {"channel", feature.Channel()},
            {"redGreenSwap", feature.RedGreenSwap()},
            {"clientBufferCount", feature.ClientBufferCount()},
            {"timeOffset", feature.TimeOffset()},
            {"bytesPerSecond", feature.Socket().GetLastBytesPerSecond()},
            {"isConnected", feature.Socket().IsConnected()},
            {"queueDepth", feature.Socket().GetCurrentQueueDepth()},
            {"queueMaxSize", feature.Socket().GetQueueMaxSize()},
            {"reconnectCount", feature.Socket().GetReconnectCount()}
        };

    const auto &response = feature.Socket().LastClientResponse();
    if (response.size == sizeof(ClientResponse))
        j["lastClientResponse"] = response;
}

// Canvas serialization
inline void to_json(nlohmann::json& j, const ICanvas & canvas) 
{
    j = {
        {"name", canvas.Name()},
        {"width", canvas.Graphics().Width()},
        {"height", canvas.Graphics().Height()},
        {"fps", canvas.Effects().GetFPS()}
    };

    // Add features array
    auto featuresJson = nlohmann::json::array();
    for (const auto& feature : canvas.Features()) {
        nlohmann::json featureJson;
        to_json(featureJson, *feature);
        featuresJson.push_back(featureJson);
    }
    j["features"] = featuresJson;
}


inline void to_json(nlohmann::json &j, const ISocketChannel & socket)
{
    try
    {
        j["hostName"] = socket.HostName();
        j["friendlyName"] = socket.FriendlyName();
        j["isConnected"] = socket.IsConnected();
        j["reconnectCount"] = socket.GetReconnectCount();
        j["queueDepth"] = socket.GetCurrentQueueDepth();
        j["queueMaxSize"] = socket.GetQueueMaxSize();
        j["bytesPerSecond"] = socket.GetLastBytesPerSecond();
        j["port"] = socket.Port();
        
        // Note: featureId and canvasId can't be included here since they're not
        // properties of the socket itself but rather of its container objects

        const auto &lastResponse = socket.LastClientResponse();
        if (lastResponse.size == sizeof(ClientResponse))
        {
            j["stats"] = lastResponse; // Uses the ClientResponse serializer
        }
    }
    catch (const std::exception &e)
    {
        j = nullptr;
    }
}
