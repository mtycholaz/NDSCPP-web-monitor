#pragma once
using namespace std;

// Serialization
//
// To be able to create a JSON representation of the LEDFeature object, we need to provide
// a to_json function that converts the object into a JSON object.  This function is then
// used by the nlohmann::json library to serialize the object.

#include <nlohmann/json.hpp>
#include "interfaces.h"

// Note that in theory, for proper separation of concerns, these functions should NOT need
// access to headers that are not part of the interfaces - i.e., everything we need should
// be on an interface

// Our Canvas class has a vector of shared pointers to ILEDFeature objects.  When we serialize
// the Canvas object, we want to serialize the ILEDFeature objects as well.  To do this, we
// need to provide a to_json function that converts the shared pointer to an ILEDFeature object
// into a JSON object.  This function is then used by the nlohmann::json library to serialize
// the shared pointer.

// ClientResponse
//
// Response data sent back to server every time we receive a packet.
// TODO: There needs to be endian-mismatch handling here if the server is
// is big-endian, as the ESP32 is little-endian.  Also you must use 64-but
// doubles on the ESP32 side to match the 64-bit doubles here on the server side.

struct ClientResponse
{
    uint32_t size;              // 4
    uint32_t flashVersion;      // 4
    double currentClock;        // 8
    double oldestPacket;        // 8
    double newestPacket;        // 8
    double brightness;          // 8
    double wifiSignal;          // 8
    uint32_t bufferSize;        // 4
    uint32_t bufferPos;         // 4
    uint32_t fpsDrawing;        // 4
    uint32_t watts;             // 4

    // Member function to translate the structure
    void TranslateClientResponse()
    {
        // Check the system's endianness
        if constexpr (std::endian::native == std::endian::little)
            return;     // No-op for little-endian systems

        // Perform byte swaps for big-endian systems
        size            = __builtin_bswap32(size);
        flashVersion    = __builtin_bswap32(flashVersion);
        currentClock    = Utilities::ByteSwapDouble(currentClock);
        oldestPacket    = Utilities::ByteSwapDouble(oldestPacket);
        newestPacket    = Utilities::ByteSwapDouble(newestPacket);
        brightness      = Utilities::ByteSwapDouble(brightness);
        wifiSignal      = Utilities::ByteSwapDouble(wifiSignal);
        bufferSize      = __builtin_bswap32(bufferSize);
        bufferPos       = __builtin_bswap32(bufferPos);
        fpsDrawing      = __builtin_bswap32(fpsDrawing);
        watts           = __builtin_bswap32(watts);
    }
};


inline void to_json(nlohmann::json& j, const ClientResponse & response)
{
    j = nlohmann::json{
        { "client-size",           response.size         },
        { "client-flashVersion",   response.flashVersion },
        { "client-currentClock",   response.currentClock },
        { "client-oldestPacket",   response.oldestPacket },
        { "client-newestPacket",   response.newestPacket },
        { "client-brightness",     response.brightness   },
        { "client-wifiSignal",     response.wifiSignal   },
        { "client-bufferSize",     response.bufferSize   },
        { "client-bufferPos",      response.bufferPos    },
        { "client-fpsDrawing",     response.fpsDrawing   },
        { "client-watts",          response.watts        }
    };
}


inline void to_json(nlohmann::json& j, const shared_ptr<ILEDFeature>& feature) 
{
    if (feature) 
    {
        // Manually serialize fields from the ILEDFeature interface
        j = nlohmann::json{
            { "hostName",     feature->HostName()     },
            { "friendlyName", feature->FriendlyName() },
            { "width",        feature->Width()        },
            { "height",       feature->Height()       },
            { "offsetX",      feature->OffsetX()      },
            { "offsetY",      feature->OffsetY()      },
            { "reversed",     feature->Reversed()     },
            { "channel",      feature->Channel()      },
            { "redGreenSwap", feature->RedGreenSwap() },
        };
    } 
    else 
    {
        j = nullptr; // Handle null shared pointers
    }
}

inline void to_json(nlohmann::json& j, const ICanvas & canvas)
{
    // Serialize the features vector as an array of JSON objects
    vector<nlohmann::json> featuresJson;
    for (const auto& feature : canvas.Features())
    {
        if (feature) 
        {
            nlohmann::json featureJson;
            to_json(featureJson, feature); // Call the shared_ptr version
            featuresJson.push_back(std::move(featureJson));
        } 
        else 
        {
            featuresJson.push_back(nullptr); // Handle null pointers
        }
    }

    j = nlohmann::json{
        { "width",    canvas.Graphics().Width()   },
        { "height",   canvas.Graphics().Height()  },
        { "features", featuresJson                }
    };
}

inline void to_json(nlohmann::json& j, const ISocketChannel& socket)
{
    j["hostName"] = socket.HostName();
    j["friendlyName"] = socket.FriendlyName();

    const auto& lastResponse = socket.LastClientResponse();
    if (lastResponse.size == sizeof(ClientResponse))
    {
        // Serialize the ClientResponse structure
        j["stats"] = {
            {"flashVersion", lastResponse.flashVersion},
            {"currentClock", lastResponse.currentClock},
            {"oldestPacket", lastResponse.oldestPacket},
            {"newestPacket", lastResponse.newestPacket},
            {"brightness", lastResponse.brightness},
            {"wifiSignal", lastResponse.wifiSignal},
            {"bufferSize", lastResponse.bufferSize},
            {"bufferPos", lastResponse.bufferPos},
            {"fpsDrawing", lastResponse.fpsDrawing},
            {"watts", lastResponse.watts}
        };
    }
}

inline void to_json(nlohmann::json& j, const shared_ptr<ISocketChannel> & socket)
{
    if (socket)
    {
        j = nlohmann::json{
            { "hostName",     socket->HostName()     },
            { "friendlyName", socket->FriendlyName() },
            { "port",         socket->Port()         }
        };
        const auto& lastResponse = socket->LastClientResponse();
        if (lastResponse.size == sizeof(ClientResponse))
        {
            // Serialize the ClientResponse structure
            j["stats"] = {
                {"flashVersion", lastResponse.flashVersion},
                {"currentClock", lastResponse.currentClock},
                {"oldestPacket", lastResponse.oldestPacket},
                {"newestPacket", lastResponse.newestPacket},
                {"brightness",   lastResponse.brightness},
                {"wifiSignal",   lastResponse.wifiSignal},
                {"bufferSize",   lastResponse.bufferSize},
                {"bufferPos",    lastResponse.bufferPos},
                {"fpsDrawing",   lastResponse.fpsDrawing},
                {"watts",        lastResponse.watts}
            };
        }        
    }
    else
    {
        j = nullptr; // Handle null shared pointers
    }
}

inline void to_json(nlohmann::json& j, const std::vector<std::shared_ptr<ISocketChannel>>& sockets)
{
    j = nlohmann::json::array();
    for (const auto& socket : sockets)
    {
        if (socket)
        {
            nlohmann::json socketJson;
            to_json(socketJson, *socket); // Use the `to_json` specialization for `ISocketChannel`
            j.push_back(socketJson);
        }
    }
}
