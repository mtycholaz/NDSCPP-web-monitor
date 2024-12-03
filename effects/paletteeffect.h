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

class PaletteEffect : public LEDEffectBase 
{
private:
    double _iPixel = 0;
    double _iColor;

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

        // Convert milliseconds to seconds for our calculations
        double secondsElapsed = deltaTime.count() / 1000.0;

        // Calculate the number of pixels to scroll based on the elapsed time
        double cPixelsToScroll = secondsElapsed * _LEDScrollSpeed;
        _iPixel += cPixelsToScroll;
        _iPixel = fmod(_iPixel, dotcount);

        // Calculate the number of colors to scroll based on the elapsed time
        double cColorsToScroll = secondsElapsed * _LEDColorPerSecond;
        _iColor += cColorsToScroll * _Density;
        _iColor -= floor(_iColor);

        double iColor = _iColor;
        uint32_t cLength = (_Mirrored ? dotcount / 2 : dotcount);

        // Draw the scrolling colors
        for (double i = 0; i < cLength; i += _EveryNthDot) 
        {
            int count = 0;
            // Draw the dots
            double iPixel = fmod(i + _iPixel, cLength);
            CRGB c = _Palette.getColor(iColor).fadeToBlackBy(1.0 - _Brightness);
            double cCenter = dotcount / 2.0;
            canvas.Graphics().SetPixelsF(iPixel + (_Mirrored ? cCenter : 0), _DotSize, c);
            if (_Mirrored) 
                canvas.Graphics().SetPixelsF(cCenter - iPixel, _DotSize, c); 
            count+= _DotSize;

            // Avoid pixel 0 flicker as it scrolls by copying pixel 1 onto 0
            if (dotcount > 1) 
                canvas.Graphics().SetPixel(0, 0, canvas.Graphics().GetPixel(1, 0));
            iColor +=  _Density / _Palette.originalSize();
        }
    }
};