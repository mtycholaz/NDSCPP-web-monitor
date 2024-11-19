#pragma once

#include <string>
#include <vector>
#include "interfaces.h"
#include "utilities.h"

class LEDFeature : public ILEDFeature
{
public:
    LEDFeature(const std::string &hostName,
               const std::string &friendlyName,
               uint32_t width,
               uint32_t height = 1,
               uint32_t offset = 0,
               bool reversed = false,
               uint8_t channel = 0,
               bool redGreenSwap = false,
               uint32_t batchSize = 1)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _width(width),
          _height(height),
          _offset(offset),
          _reversed(reversed),
          _channel(channel),
          _redGreenSwap(redGreenSwap),
          _batchSize(batchSize),
          _pixels(width * height, CRGB::Black)
    {
    }

    // Accessor methods
    uint32_t Width() const override { return _width; }
    uint32_t Height() const override { return _height; }
    const std::string &HostName() const override { return _hostName; }
    const std::string &FriendlyName() const override { return _friendlyName; }
    uint32_t Offset() const override { return _offset; }
    bool Reversed() const override { return _reversed; }
    uint8_t Channel() const override { return _channel; }
    bool RedGreenSwap() const override { return _redGreenSwap; }
    uint32_t BatchSize() const override { return _batchSize; }

    // Data retrieval
    std::vector<uint8_t> GetPixelData() const override
    {
        return Utilities::GetColorBytesAtOffset(_pixels, _offset, _width * _height, _reversed, _redGreenSwap);
    }

    // Effect handling
    void ApplyEffect(ILEDEffect &effect, std::chrono::milliseconds deltaTime) override
    {
    }

    // State handling
    void Clear() override
    {
        std::fill(_pixels.begin(), _pixels.end(), CRGB::Black);
    }

private:
    std::string _hostName;
    std::string _friendlyName;
    uint32_t _width;
    uint32_t _height;
    uint32_t _offset;
    bool _reversed;
    uint8_t _channel;
    bool _redGreenSwap;
    uint32_t _batchSize;
    std::vector<CRGB> _pixels; // Internal storage for pixel data
};
