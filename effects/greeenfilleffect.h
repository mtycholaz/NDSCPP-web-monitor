#pragma once

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"

class GreenFillEffect : public LEDEffectBase
{
public:

    GreenFillEffect(const std::string& name) : LEDEffectBase(name) 
    {
    }

    void Start(ICanvas& canvas) override
    {
    }

    void Update(ICanvas& canvas, std::chrono::milliseconds deltaTime) override
    {
        canvas.Graphics().Clear(CRGB::Green);
    }
};
