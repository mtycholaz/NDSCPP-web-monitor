#pragma once

// BaseGraphics
//
// A class that can do all the basic drawing functions (pixel, line, circle, etc) of the ILEDGraphics 
// interface to its own internal buffer of pixels.  It manipulates the buffer only through SetPixel

#include <vector>
#include "pixeltypes.h" // Assuming this defines the CRGB structure
#include "interfaces.h"

class BaseGraphics : public ILEDGraphics
{
protected:
    int _width;
    int _height;
    std::vector<CRGB> _pixels;

    virtual int _index(int x, int y) const 
    {
        return y * _width + x;
    }

public:
    explicit BaseGraphics(int width, int height) 
        : _width(width), _height(height), _pixels(width * height) 
    {

    }

    // ICanvas methods
    virtual uint32_t Width()  const override { return _width; }
    virtual uint32_t Height() const override { return _height; }

    virtual void SetPixel(int x, int y, const CRGB& color) override
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
            _pixels[_index(x, y)] = color;
    }

    virtual CRGB GetPixel(int x, int y) const override 
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
            return _pixels[_index(x, y)];
        return CRGB(0, 0, 0); // Default to black for out-of-bounds
    }

    virtual void Clear(const CRGB& color = CRGB::Black) override
    {
        FillRectangle(0, 0, _width, _height, color);
    }

    virtual void FillRectangle(int x, int y, int width, int height, const CRGB& color) override
    {
        for (int j = y; j < y + height; ++j)
            for (int i = x; i < x + width; ++i)
                SetPixel(i, j, color);
    }

    virtual void DrawLine(int x1, int y1, int x2, int y2, const CRGB& color) override
    {
        int dx = abs(x2 - x1), dy = abs(y2 - y1);
        int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
        int err = dx - dy;

        while (true) {
            SetPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }
        }
    }

    virtual void DrawCircle(int x, int y, int radius, const CRGB& color) override
    {
        int cx = 0, cy = radius, d = 1 - radius;

        while (cy >= cx) {
            SetPixel(x + cx, y + cy, color);
            SetPixel(x - cx, y + cy, color);
            SetPixel(x + cx, y - cy, color);
            SetPixel(x - cx, y - cy, color);
            SetPixel(x + cy, y + cx, color);
            SetPixel(x - cy, y + cx, color);
            SetPixel(x + cy, y - cx, color);
            SetPixel(x - cy, y - cx, color);

            ++cx;
            if (d < 0)
                d += 2 * cx + 1;
            else {
                --cy;
                d += 2 * (cx - cy) + 1;
            }
        }
    }

    virtual void FillCircle(int x, int y, int radius, const CRGB& color) override
    {
        for (int cy = -radius; cy <= radius; ++cy)
            for (int cx = -radius; cx <= radius; ++cx)
                if (cx * cx + cy * cy <= radius * radius)
                    SetPixel(x + cx, y + cy, color);
    }

    virtual void DrawRectangle(int x, int y, int width, int height, const CRGB& color) override
    {
        DrawLine(x, y, x + width - 1, y, color);             // Top
        DrawLine(x, y, x, y + height - 1, color);            // Left
        DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color); // Right
        DrawLine(x, y + height - 1, x + width - 1, y + height - 1, color); // Bottom
    }
};
