#pragma once

#include "interfaces.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>

class Canvas : public ICanvas, public ILEDGraphics
{
public:
    Canvas(uint32_t width, uint32_t height)
        : _width(width), _height(height), _leds(width * height) {}

    // ICanvas methods
    uint32_t Width() const override { return _width; }
    uint32_t Height() const override { return _height; }

    std::vector<std::shared_ptr<ILEDFeature>>& Features() override
    {
        return _features;
    }

    const std::vector<std::shared_ptr<ILEDFeature>>& Features() const override
    {
        return _features;
    }

    void AddFeature(std::shared_ptr<ILEDFeature> feature) override
    {
        if (!feature)
            throw std::invalid_argument("Cannot add a null feature.");

        _features.push_back(feature);
    }

    void RemoveFeature(std::shared_ptr<ILEDFeature> feature) override
    {
        if (!feature)
            throw std::invalid_argument("Cannot remove a null feature.");

        auto it = std::find(_features.begin(), _features.end(), feature);
        if (it != _features.end())
            _features.erase(it);
    }

    // ILEDGraphics methods
    CRGB GetPixel(uint32_t x, uint32_t y) const override
    {
        if (x >= _width || y >= _height)
            return CRGB::Black;

        return _leds[y * _width + x];
    }

    void DrawPixel(uint32_t x, uint32_t y, CRGB color) override
    {
        if (x < _width && y < _height)
            _leds[y * _width + x] = color;
    }

    void DrawPixels(double position, double count, CRGB color) override
    {
        int start = static_cast<int>(position);
        int end = static_cast<int>(position + count);

        for (int i = start; i < end && i < static_cast<int>(_leds.size()); ++i)
            _leds[i] = color;
    }

    void DrawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, CRGB color) override
    {
        // Bresenham's line algorithm
        int dx = abs((int)x1 - (int)x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs((int)y1 - (int)y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true)
        {
            DrawPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1)
                break;
            e2 = 2 * err;
            if (e2 >= dy)
            {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx)
            {
                err += dx;
                y0 += sy;
            }
        }
    }

    void DrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, CRGB color) override
    {
        for (uint32_t i = x; i < x + w && i < _width; ++i)
            for (uint32_t j = y; j < y + h && j < _height; ++j)
                DrawPixel(i, j, color);
    }

    void DrawCircle(uint32_t x0, uint32_t y0, uint32_t radius, CRGB color) override
    {
        int f = 1 - radius;
        int ddF_x = 1;
        int ddF_y = -2 * radius;
        int x = 0;
        int y = radius;

        DrawPixel(x0, y0 + radius, color);
        DrawPixel(x0, y0 - radius, color);
        DrawPixel(x0 + radius, y0, color);
        DrawPixel(x0 - radius, y0, color);

        while (x < y)
        {
            if (f >= 0)
            {
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            DrawPixel(x0 + x, y0 + y, color);
            DrawPixel(x0 - x, y0 + y, color);
            DrawPixel(x0 + x, y0 - y, color);
            DrawPixel(x0 - x, y0 - y, color);
            DrawPixel(x0 + y, y0 + x, color);
            DrawPixel(x0 - y, y0 + x, color);
            DrawPixel(x0 + y, y0 - x, color);
            DrawPixel(x0 - y, y0 - x, color);
        }
    }

    void FillSolid(CRGB color) override
    {
        std::fill(_leds.begin(), _leds.end(), color);
    }

    void FillRainbow(double startHue, double deltaHue) override
    {
        double hue = startHue;
        for (uint32_t y = 0; y < _height; ++y)
        {
            for (uint32_t x = 0; x < _width; ++x)
            {
                DrawPixel(x, y, CRGB::HSV2RGB(hue));
                hue += deltaHue;
            }
        }
    }

private:
    uint32_t _width;
    uint32_t _height;
    std::vector<CRGB> _leds;
    std::vector<std::shared_ptr<ILEDFeature>> _features;

    /*
    CRGB GetPixel(int index, int) const
    {
        if (index < 0 || static_cast<size_t>(index) >= _leds.size())
            return CRGB::Black;
        return _leds[index];
    }
    */
};
