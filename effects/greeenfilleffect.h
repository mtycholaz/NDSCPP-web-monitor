#pragma once

// GreenFillEffect
// 
// A simple effect that fills the entire canvas with green for testing purposes.

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"

class GreenFillEffect : public LEDEffectBase
{
public:

    GreenFillEffect(const std::string& name) : LEDEffectBase(name) 
    {
    }

    void Update(ICanvas& canvas, std::chrono::milliseconds deltaTime) override
    {
        canvas.Graphics().Clear(CRGB::Green);
    }
};
