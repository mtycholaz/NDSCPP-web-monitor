#pragma once

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"

class GreenFillEffect : public LEDEffectBase
{
public:
    void Start(ICanvas& canvas) override
    {
    }

    void Update(ICanvas& canvas, std::chrono::milliseconds deltaTime) override
    {
        canvas.Graphics().FillSolid(CRGB::Green);
    }
};
