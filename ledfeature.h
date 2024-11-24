#pragma once
using namespace std;
using namespace std::chrono;

#include <nlohmann/json.hpp>

// LEDFeature
//
// Represents one rectangular section of the canvas and is responsible for producing the
// color data frames for that section of the canvas.  The LEDFeature is associated with a
// specific Canvas object, and it retrieves the pixel data from the Canvas to produce the
// data frame.  The LEDFeature is also responsible for producing the data frame in the
// format that the ESP32 expects.

#include "interfaces.h"
#include "utilities.h"
#include "canvas.h"
#include <string>

class LEDFeature : public ILEDFeature
{
public:
    LEDFeature(Canvas * canvas,
               const string &hostName,
               const string &friendlyName,
               uint16_t port,
               uint32_t width,
               uint32_t height = 1,
               uint32_t offsetX = 0,
               uint32_t offsetY = 0,
               bool reversed = false,
               uint8_t channel = 0,
               bool redGreenSwap = false)
        : _canvas(canvas),
          _width(width),
          _height(height),
          _offsetX(offsetX),
          _offsetY(offsetY),
          _reversed(reversed),
          _channel(channel),
          _redGreenSwap(redGreenSwap),
          _socketChannel(hostName, friendlyName, port)
    {
    }

    // Accessor methods
    uint32_t        Width()        const override { return _width; }
    uint32_t        Height()       const override { return _height; }
    uint32_t        OffsetX()      const override { return _offsetX; }
    uint32_t        OffsetY()      const override { return _offsetY; }
    bool            Reversed()     const override { return _reversed; }
    uint8_t         Channel()      const override { return _channel; }
    bool            RedGreenSwap() const override { return _redGreenSwap; }

    virtual ISocketChannel & Socket() override 
    {
        return _socketChannel;
    }

   // Data retrieval
    vector<uint8_t> GetPixelData() const override
    {
        if (!_canvas)
            throw runtime_error("LEDFeature must be associated with a canvas to retrieve pixel data.");

        const auto& graphics = _canvas->Graphics();

        // If the feature matches the canvas in size and position, directly return the canvas pixel data
        if (_width == graphics.Width() && _height == graphics.Height() && _offsetX == 0 && _offsetY == 0)
        {
            return Utilities::ConvertPixelsToByteArray(
                graphics.GetPixels(),
                _reversed,
                _redGreenSwap
            );
        }

        // Otherwise, manually extract the feature's pixel data
        vector<CRGB> featurePixels(_width * _height, CRGB::Magenta); // Initialize with Magenta by default

        for (uint32_t y = 0; y < _height; ++y)
        {
            for (uint32_t x = 0; x < _width; ++x)
            {
                // Map feature (local) coordinates to canvas (global) coordinates
                uint32_t canvasX = x + _offsetX;
                uint32_t canvasY = y + _offsetY;

                // Calculate index for the current pixel in featurePixels
                uint32_t featureIndex = y * _width + x;

                // Ensure we don't exceed canvas boundaries
                if (canvasX < graphics.Width() && canvasY < graphics.Height())
                    featurePixels[featureIndex] = graphics.GetPixel(canvasX, canvasY);
            }
        }

        return Utilities::ConvertPixelsToByteArray(featurePixels, _reversed, _redGreenSwap);
    }

    vector<uint8_t> GetDataFrame() const override
    {
        // Calculate epoch time
        auto now = system_clock::now();
        auto epoch = duration_cast<microseconds>(now.time_since_epoch()).count();
        uint64_t seconds = epoch / 1'000'000 + 2;
        uint64_t microseconds = epoch % 1'000'000;

        auto pixelData = GetPixelData();

        return Utilities::CombineByteArrays(Utilities::WORDToBytes(3),
                                            Utilities::WORDToBytes(_channel),
                                            Utilities::DWORDToBytes(_width * _height),
                                            Utilities::ULONGToBytes(seconds),
                                            Utilities::ULONGToBytes(microseconds),
                                            pixelData);
    }

private:
    uint32_t    _width;
    uint32_t    _height;
    uint32_t    _offsetX;
    uint32_t    _offsetY;
    bool        _reversed;
    uint8_t     _channel;
    bool        _redGreenSwap;
    ICanvas   * _canvas; // Associated canvas
    SocketChannel _socketChannel;
};
