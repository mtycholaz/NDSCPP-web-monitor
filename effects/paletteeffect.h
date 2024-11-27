#pragma once

// PaletteEffect
// 
// A versatile effect that scrolls a palette of colors across the canvas.  The effect can be configured
// with a variety of parameters to control the speed, density, and appearance of the scrolling colors.

using namespace std;
using namespace std::chrono;

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"
#include "../palette.h"

constexpr auto kPixelsPerMeter = 144;

class PaletteEffect : public LEDEffectBase 
{
private:
    double _iPixel = 0;
    double _iColor;
    std::chrono::system_clock::time_point _lastDraw;

public:
    Palette  _Palette;
    double   _LEDColorPerSecond = 3.0;
    double   _LEDScrollSpeed = 0.0;
    double   _Density = 1.0;
    double   _EveryNthDot = 1.0;
    uint32_t _DotSize = 1;
    bool     _RampedColor = false;
    double   _Brightness = 1.0;
    bool     _Mirrored = false;

    PaletteEffect(const string & name, 
                  const Palette& palette,
                  double         ledColorPerSecond = 3.0,
                  double         ledScrollSpeed = 0.0,
                  double         density = 1.0,
                  double         everyNthDot = 1.0,
                  uint32_t       dotSize = 1,
                  bool           rampedColor = false,
                  double         brightness = 1.0,
                  bool           mirrored = false)
        : _Palette(palette)
        , _iColor(0)
        , _lastDraw(std::chrono::system_clock::now())
        , _LEDColorPerSecond(ledColorPerSecond)
        , _LEDScrollSpeed(ledScrollSpeed)
        , _Density(density)
        , _EveryNthDot(everyNthDot)
        , _DotSize(dotSize)
        , _RampedColor(rampedColor)
        , _Brightness(brightness)
        , _Mirrored(mirrored)
        , LEDEffectBase(name)
    {
    }

    void Update(ICanvas& canvas, milliseconds deltaTime) override 
    {
        auto dotcount = canvas.Graphics().Width() * canvas.Graphics().Height();

        canvas.Graphics().Clear(CRGB::Black);

        // Calculate the number of pixels to scroll based on the elapsed time
        auto now = std::chrono::system_clock::now();
        double secondsElapsed = std::chrono::duration<double>(now - _lastDraw).count();
        _lastDraw = now;

        double cPixelsToScroll = secondsElapsed * _LEDScrollSpeed;
        _iPixel += cPixelsToScroll;
        _iPixel = fmod(_iPixel, dotcount);

        // Calculate the number of colors to scroll based on the elapsed time
        double cColorsToScroll = secondsElapsed * _LEDColorPerSecond;
        _iColor += cColorsToScroll / kPixelsPerMeter;
        _iColor -= floor(_iColor);

        double iColor = _iColor;
        uint32_t cLength = (_Mirrored ? dotcount / 2 : dotcount);

        // Draw the scrolling colors
        for (double i = 0; i < cLength; i += _EveryNthDot) 
        {
            int count = 0;
            // Draw the dots
            for (uint32_t j = 0; j < _DotSize && (i + j) < cLength; j++) 
            {
                double iPixel = fmod(i + j + _iPixel, cLength);
                CRGB c = _Palette.getColor(iColor).fadeToBlackBy(1.0 - _Brightness);
                double cCenter = dotcount / 2.0;
                canvas.Graphics().SetPixel(iPixel + (_Mirrored ? cCenter : 0), 0, c);
                if (_Mirrored) 
                    canvas.Graphics().SetPixel(cCenter - iPixel, 1, c);
                count++;
            }

            // Avoid pixel 0 flicker as it scrolls by copying pixel 1 onto 0
            if (dotcount > 1) 
                canvas.Graphics().SetPixel(0, 0, canvas.Graphics().GetPixel(1, 0));
            iColor += count * (_Density / kPixelsPerMeter) * _EveryNthDot;
        }
    }
};