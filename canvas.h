#pragma once

// Canvas
//
// Represents the larger drawing surface that is made up from one or more LEDFeatures

#include "interfaces.h"
#include "basegraphics.h"
#include "effectsmanager.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>
#include <numeric>


class Canvas : public ICanvas
{
    BaseGraphics _graphics;
    EffectsManager _effects;
    
public:
    Canvas(uint32_t width, uint32_t height) : _width(width), _height(height), _graphics(width, height)
    {
    }

    ILEDGraphics & Graphics() override
    {
        return _graphics;
    }
    
    EffectsManager & Effects() 
    {
        return _effects;
    }

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

private:
    uint32_t _width;
    uint32_t _height;
    std::vector<std::shared_ptr<ILEDFeature>> _features;
};
