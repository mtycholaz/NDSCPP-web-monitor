#pragma once

// EffectsManager
//
// Manages a collection of ILEDEffect objects.  The EffectsManager is responsible for
// starting and stopping the effects, and for switching between them.  The EffectsManager
// can also be used to clear all effects.

#include "interfaces.h"
#include <vector>
#include <memory>
#include <stdexcept>
#include <chrono>

class EffectsManager
{
public:
    EffectsManager()
        : _currentEffectIndex(-1) // No effect selected initially
    {}

    ~EffectsManager() = default;

    // Add an effect to the manager
    void AddEffect(std::shared_ptr<ILEDEffect> effect)
    {
        if (!effect)
            throw std::invalid_argument("Cannot add a null effect.");
        _effects.push_back(effect);

        // Automatically set the first effect as current if none is selected
        if (_currentEffectIndex == -1)
            _currentEffectIndex = 0;
    }

    // Remove an effect from the manager
    void RemoveEffect(std::shared_ptr<ILEDEffect> effect)
    {
        if (!effect)
            throw std::invalid_argument("Cannot remove a null effect.");

        auto it = std::remove(_effects.begin(), _effects.end(), effect);
        if (it != _effects.end())
        {
            auto index = std::distance(_effects.begin(), it);
            _effects.erase(it);

            // Adjust the current effect index
            if (index <= _currentEffectIndex)
                _currentEffectIndex = (_currentEffectIndex > 0) ? _currentEffectIndex - 1 : -1;

            // If no effects remain, reset the current index
            if (_effects.empty())
                _currentEffectIndex = -1;
        }
    }

    // Start the current effect
    void StartCurrentEffect(ICanvas& canvas)
    {
        if (IsEffectSelected())
            _effects[_currentEffectIndex]->Start(canvas);
    }

    void SetCurrentEffect(size_t index, ICanvas& canvas)
    {
        if (index >= _effects.size())
            throw std::out_of_range("Effect index out of range.");

        _currentEffectIndex = index;

        StartCurrentEffect(canvas);
   }

    // Update the current effect and render it to the canvas
    void UpdateCurrentEffect(ICanvas& canvas, std::chrono::milliseconds millisDelta)
    {
        if (IsEffectSelected())
            _effects[_currentEffectIndex]->Update(canvas, millisDelta);
    }

    // Switch to the next effect
    void NextEffect()
    {
        if (!_effects.empty())
            _currentEffectIndex = (_currentEffectIndex + 1) % _effects.size();
    }

    // Switch to the previous effect
    void PreviousEffect()
    {
        if (!_effects.empty())
            _currentEffectIndex = (_currentEffectIndex == 0) ? _effects.size() - 1 : _currentEffectIndex - 1;
    }

    // Get the name of the current effect
    std::string CurrentEffectName() const
    {
        if (IsEffectSelected())
            return _effects[_currentEffectIndex]->Name();
        return "No Effect Selected";
    }

    // Clear all effects
    void ClearEffects()
    {
        _effects.clear();
        _currentEffectIndex = -1;
    }

private:
    std::vector<std::shared_ptr<ILEDEffect>> _effects;
    int _currentEffectIndex; // Index of the current effect

    bool IsEffectSelected() const
    {
        return _currentEffectIndex >= 0 && _currentEffectIndex < static_cast<int>(_effects.size());
    }
};
