#pragma once

#include "interfaces.h"
#include "utilities.h"
#include "canvas.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

class LEDFeature : public ILEDFeature
{
public:
    LEDFeature(const std::string &hostName,
               const std::string &friendlyName,
               uint32_t width,
               uint32_t height = 1,
               uint32_t offsetX = 0,
               uint32_t offsetY = 0,
               bool reversed = false,
               uint8_t channel = 0,
               bool redGreenSwap = false,
               uint32_t batchSize = 1)
        : _hostName(hostName),
          _friendlyName(friendlyName),
          _width(width),
          _height(height),
          _offsetX(offsetX),
          _offsetY(offsetY),
          _reversed(reversed),
          _channel(channel),
          _redGreenSwap(redGreenSwap),
          _batchSize(batchSize),
          _canvas(nullptr) {}

    // Accessor methods
    uint32_t Width() const override { return _width; }
    uint32_t Height() const override { return _height; }
    const std::string &HostName() const override { return _hostName; }
    const std::string &FriendlyName() const override { return _friendlyName; }
    virtual uint32_t OffsetX() const override { return _offsetX; }
    virtual uint32_t OffsetY() const override { return _offsetY; }
    bool Reversed() const override { return _reversed; }
    uint8_t Channel() const override { return _channel; }
    bool RedGreenSwap() const override { return _redGreenSwap; }
    uint32_t BatchSize() const override { return _batchSize; }

    // Canvas association
    void SetCanvas(std::shared_ptr<ICanvas> canvas) { _canvas = canvas; }
    std::shared_ptr<ICanvas> GetCanvas() const { return _canvas; }

    // Data retrieval
    std::vector<uint8_t> GetPixelData() const override
    {
        if (!_canvas)
            throw std::runtime_error("LEDFeature must be associated with a canvas to retrieve pixel data.");

        std::vector<CRGB> featurePixels;

        // Retrieve pixel data from the canvas
        for (uint32_t y = 0; y < _height; ++y)
        {
            for (uint32_t x = 0; x < _width; ++x)
            {
                // Map feature (local) coordinates to canvas (global) coordinates
                uint32_t canvasX = x + _offsetX;
                uint32_t canvasY = y + _offsetY;

                // Ensure we don't exceed canvas boundaries
                if (canvasX < _canvas->Width() && canvasY < _canvas->Height())
                    featurePixels.push_back(_canvas->GetPixel(canvasX, canvasY));
                else
                    featurePixels.push_back(CRGB::Magenta);           // Out-of-bounds pixels are defaulted to Magenta
            }
        }

        // Convert to the desired byte array format (e.g., RGB)
        return Utilities::ConvertToByteArray(featurePixels, _reversed, _redGreenSwap);
    }

    // State handling
    void Clear() override
    {
        if (!_canvas)
            throw std::runtime_error("LEDFeature must be associated with a canvas to clear.");

        for (uint32_t y = 0; y < _height; ++y)
        {
            for (uint32_t x = 0; x < _width; ++x)
            {
                uint32_t canvasX = x + _offsetX;
                uint32_t canvasY = y + _offsetY;

                if (canvasX < _canvas->Width() && canvasY < _canvas->Height())
                    _canvas->DrawPixel(canvasX, canvasY, CRGB::Black);
            }
        }
    }

private:
    std::string _hostName;
    std::string _friendlyName;
    uint32_t _width;
    uint32_t _height;
    uint32_t _offsetX;
    uint32_t _offsetY;
    bool _reversed;
    uint8_t _channel;
    bool _redGreenSwap;
    uint32_t _batchSize;

    std::shared_ptr<ICanvas> _canvas; // Associated canvas
};
