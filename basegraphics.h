#pragma once
using namespace std;

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
    uint32_t _width;
    uint32_t _height;
    vector<CRGB> _pixels;

    virtual inline __attribute__((always_inline)) uint32_t _index(uint32_t x, uint32_t y) const 
    {
        return y * _width + x;
    }

public:
    explicit BaseGraphics(uint32_t width, uint32_t height) 
        : _width(width), _height(height), _pixels(width * height) 
    {

    }

    // ICanvas methods
    uint32_t Width()  const override { return _width; }
    uint32_t Height() const override { return _height; }

    const vector<CRGB>& GetPixels() const override
    {
        return _pixels;
    }

    void SetPixel(uint32_t x, uint32_t y, const CRGB& color) override
    {
        if (x < _width && y < _height)
            _pixels[_index(x, y)] = color;
    }

    CRGB GetPixel(uint32_t x, uint32_t y) const override 
    {
        if (x < _width && y < _height)
            return _pixels[_index(x, y)];
        return CRGB(0, 0, 0); // Default to black for out-of-bounds
    }

    void Clear(const CRGB& color = CRGB::Black) override
    {
        FillRectangle(0, 0, _width, _height, color);
    }

    void FillRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const CRGB& color) override
    {
        for (int j = y; j < y + height; ++j)
            for (int i = x; i < x + width; ++i)
                SetPixel(i, j, color);
    }

    void DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, const CRGB& color) override
    {
        int32_t dx = abs((int32_t)x2 - (int32_t)x1), dy = abs((int32_t)y2 - (int32_t)y1);
        int32_t sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
        int32_t err = dx - dy;

        while (true) 
        {
            SetPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            int e2 = 2 * err;
            if (e2 > -dy) 
            {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) 
            {
                err += dx;
                y1 += sy;
            }
        }
    }

    void DrawCircle(uint32_t x, uint32_t y, uint32_t radius, const CRGB& color) override
    {
        uint32_t cx = 0, cy = radius;
        int32_t  d = 1 - radius;

        while (cy >= cx) 
        {
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
            {
                d += 2 * cx + 1;
            }
            else 
            {
                --cy;
                d += 2 * (cx - cy) + 1;
            }
        }
    }

    void FillCircle(uint32_t x, uint32_t y, uint32_t radius, const CRGB& color) override
    {
        // Fill within the bounding box, but only those pixels within radius of the center

        for (int32_t cy = -radius; cy <= radius; ++cy)
            for (int32_t cx = -radius; cx <= radius; ++cx)
                if (cx * cx + cy * cy <= radius * radius)
                    SetPixel(x + cx, y + cy, color);
    }

    void DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const CRGB& color) override
    {
        DrawLine(x, y, x + width - 1, y, color);                            // Top
        DrawLine(x, y, x, y + height - 1, color);                           // Left
        DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color);   // Right
        DrawLine(x, y + height - 1, x + width - 1, y + height - 1, color);  // Bottom
    }

    void FadeFrameBy(uint8_t dimAmount) override
    {
        for (auto& pixel : _pixels)
        {
            pixel.r = scale8(pixel.r, 255-dimAmount);
            pixel.g = scale8(pixel.g, 255-dimAmount);
            pixel.b = scale8(pixel.b, 255-dimAmount);
        }
    }

    void SetPixelsF(float fPos, float count, CRGB c, bool bMerge = false) override
    {
        float frac1 = fPos - floor(fPos);                 // eg:   3.25 becomes 0.25
        float frac2 = fPos + count - floor(fPos + count); // eg:   3.25 + 1.5 yields 4.75 which becomes 0.75

        /* Example:

          Starting at 3.25, draw for 1.5:
          We start at pixel 3.
          We fill pixel with .75 worth of color
          We advance to next pixel

          We fill one pixel and advance to next pixel

          We are now at pixel 5, frac2 = .75
          We fill pixel with .75 worth of color
        */

        uint8_t fade1 = (uint8_t) ((std::max(frac1, 1.0f - count)) * 255); // Fraction is how far past pixel boundary we are (up to our total size) so larger fraction is more dimming
        uint8_t fade2 = (uint8_t) ((1.0f - frac2) * 255);                   // Fraction is how far we are poking into this pixel, so larger fraction is less dimming
        CRGB c1 = c;
        CRGB c2 = c;
        c1 = c1.fadeToBlackBy(fade1);
        c2 = c2.fadeToBlackBy(fade2);

        // These assignments use the + operator of CRGB to merge the colors when requested, and it's pretty
        // naive, just saturating each color element at 255, so the operator could be improved or replaced
        // if needed...

        float p = fPos;
        if (p >= 0 && p < _pixels.size())
            _pixels[(int)p] = bMerge ? _pixels[(int)p] + c1 : c1;

        p = fPos + (1.0f - frac1);
        count -= (1.0f - frac1);

        // Middle (body) pixels

        while (count >= 1)
        {
            if (p >= 0 && p < _pixels.size())
                _pixels[(int)p] = bMerge ? _pixels[(int)p] + c : c;
            count--;
            p++;
        };

        // Final pixel, if in bounds
        if (count > 0)
            if (p >= 0 && p < _pixels.size())
                _pixels[(int)p] = bMerge ? _pixels[(int)p] + c2 : c2;
    }    
};
