#pragma once
using namespace std;

// Canvas
//
// Represents the larger drawing surface that is made up from one or more LEDFeatures

#include <nlohmann/json.hpp>
#include "interfaces.h"
#include "basegraphics.h"
#include "effectsmanager.h"
#include <vector>



class Canvas : public ICanvas
{
    BaseGraphics _graphics;
    EffectsManager _effects;
    
public:
    Canvas(uint32_t width, uint32_t height, uint16_t fps = 30) : 
        _width(width), 
        _height(height), 
        _graphics(width, height), 
        _effects(fps)
    {
    }

    ILEDGraphics & Graphics() override
    {
        return _graphics;
    }
    
    const ILEDGraphics& Graphics() const override 
    { 
        return _graphics; 
    }

    IEffectsManager & Effects() override
    {
        return _effects;
    }

    vector<unique_ptr<ILEDFeature>>& Features() override
    {
        return _features;
    }

    const vector<unique_ptr<ILEDFeature>>& Features() const override
    {
        return _features;
    }

    void AddFeature(unique_ptr<ILEDFeature> feature) override
    {
        if (!feature)
            throw invalid_argument("Cannot add a null feature.");

        _features.push_back(std::move(feature));
    }

    void RemoveFeature(unique_ptr<ILEDFeature> feature) override
    {
        if (!feature)
            throw invalid_argument("Cannot remove a null feature.");

        auto it = find(_features.begin(), _features.end(), feature);
        if (it != _features.end())
            _features.erase(it);
    }

private:
    uint32_t _width;
    uint32_t _height;
    vector<unique_ptr<ILEDFeature>> _features;
};
