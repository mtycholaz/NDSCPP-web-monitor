#pragma once

// LEDEffectBase
// 
// A helper class that implements the ILEDEffect interface.  

#include "interfaces.h"
#include <vector>
#include "pixeltypes.h"

class LEDEffectBase : public ILEDEffect
{
protected:
    std::string _name;

public:
    LEDEffectBase(const std::string& name) : _name(name) {}

    virtual ~LEDEffectBase() = default;

    const std::string& Name() const override { return _name; }

    // Default implementation for Start does nothing
    virtual void Start(ICanvas& canvas) override 
    {
    }

    // Default implementation for Update does nothing
    void Update(ICanvas& canvas, std::chrono::milliseconds deltaTime) override 
    {
    }
};
