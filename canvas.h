#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include "interfaces.h"
#include "utilities.h"
#include "pixeltypes.h"

class Canvas : public ICanvas, public ILEDGraphics
{
public:
    Canvas(const std::string& name, uint32_t width, uint32_t height)
        : _name(name), _width(width), _height(height), _leds(width * height, CRGB::Black) {}

    // ICanvas overrides
    const std::string& Name() const override { return _name; }
    uint32_t Width() const override { return _width; }
    uint32_t Height() const override { return _height; }
    void AddFeature(std::shared_ptr<ILEDFeature> feature, uint32_t x, uint32_t y) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _features[feature] = {x, y};
    }
    void RemoveFeature(std::shared_ptr<ILEDFeature> feature) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _features.erase(feature);
    }
    const std::map<std::shared_ptr<ILEDFeature>, std::pair<uint32_t, uint32_t>>& Features() const override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _features;
    }

    // ILEDGraphics overrides
    void DrawPixel(uint32_t x, uint32_t y, const CRGB& color) override
    {
        if (x < _width && y < _height) {
            std::lock_guard<std::mutex> lock(_mutex);
            _leds[y * _width + x] = color;
        }
    }
    void DrawPixel(uint32_t index, const CRGB& color) override
    {
        if (index < _leds.size()) {
            std::lock_guard<std::mutex> lock(_mutex);
            _leds[index] = color;
        }
    }
    CRGB GetPixel(uint32_t x, uint32_t y) const override
    {
        if (x < _width && y < _height) {
            std::lock_guard<std::mutex> lock(_mutex);
            return _leds[y * _width + x];
        }
        return CRGB::Black;
    }
    CRGB GetPixel(uint32_t index) const override
    {
        if (index < _leds.size()) {
            std::lock_guard<std::mutex> lock(_mutex);
            return _leds[index];
        }
        return CRGB::Black;
    }
    void BlendPixel(uint32_t x, uint32_t y, const CRGB& color) override
    {
        if (x < _width && y < _height) {
            std::lock_guard<std::mutex> lock(_mutex);
            uint32_t index = y * _width + x;
            _leds[index] = _leds[index] + color;
        }
    }
    void DrawFastVLine(uint32_t x, uint32_t y, uint32_t h, const CRGB& color) override
    {
        for (uint32_t i = 0; i < h; ++i)
            DrawPixel(x, y + i, color);
    }
    void DrawFastHLine(uint32_t x, uint32_t y, uint32_t w, const CRGB& color) override
    {
        for (uint32_t i = 0; i < w; ++i)
            DrawPixel(x + i, y, color);
    }
    void DrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const CRGB& color) override
    {
        DrawFastHLine(x, y, w, color);
        DrawFastHLine(x, y + h - 1, w, color);
        DrawFastVLine(x, y, h, color);
        DrawFastVLine(x + w - 1, y, h, color);
    }
    void FillSolid(const CRGB& color) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& led : _leds)
            led = color;
    }
    void FillRainbow(double startHue = 0.0, double deltaHue = 5.0) override
    {
        std::lock_guard<std::mutex> lock(_mutex);
        double hue = startHue;
        for (auto& led : _leds) {
            led = CRGB::HSV2RGB(hue, 1.0, 1.0);
            hue += deltaHue;
            if (hue > 360.0) hue -= 360.0;
        }
    }

private:
    std::string _name;
    uint32_t _width;
    uint32_t _height;
    mutable std::mutex _mutex;
    std::vector<CRGB> _leds;
    std::map<std::shared_ptr<ILEDFeature>, std::pair<uint32_t, uint32_t>> _features;
};
