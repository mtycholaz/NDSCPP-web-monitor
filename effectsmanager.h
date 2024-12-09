#pragma once
using namespace std;
using namespace std::chrono;

#include "effects/colorwaveeffect.h"
#include "effects/fireworkseffect.h"
#include "effects/misceffects.h"
#include "effects/paletteeffect.h"
#include "effects/starfield.h"
#include "effects/videoeffect.h"

// EffectsManager
//
// Manages a collection of ILEDEffect objects.  The EffectsManager is responsible for
// starting and stopping the effects, and for switching between them.  The EffectsManager
// can also be used to clear all effects.

#include "interfaces.h"
#include <vector>

class EffectsManager : public IEffectsManager
{
public:
    uint16_t _fps;
    int _currentEffectIndex; // Index of the current effect
    atomic<bool> _running;
    vector<unique_ptr<ILEDEffect>> _effects;
    thread _workerThread;

    EffectsManager(uint16_t fps = 30) : _fps(fps), _currentEffectIndex(-1), _running(false) // No effect selected initially
    {
    }

    ~EffectsManager()
    {
        Stop(); // Ensure the worker thread is stopped when the manager is destroyed
    }

    void SetFPS(uint16_t fps) override
    {
        _fps = fps;
    }

    uint16_t GetFPS() const override
    {
        return _fps;
    }

    size_t GetCurrentEffect() const override
    {
        return _currentEffectIndex;
    }

    size_t EffectCount() const override
    {
        return _effects.size();
    }

    vector<reference_wrapper<ILEDEffect>> Effects() const override
    {
        vector<reference_wrapper<ILEDEffect>> effects;
        effects.reserve(_effects.size());
        for (auto &effect : _effects)
            effects.push_back(*effect);
        return effects;
    }

    // Add an effect to the manager
    void AddEffect(unique_ptr<ILEDEffect> effect) override
    {
        if (!effect)
            throw invalid_argument("Cannot add a null effect.");
        _effects.push_back(std::move(effect));

        // Automatically set the first effect as current if none is selected
        if (_currentEffectIndex == -1)
            _currentEffectIndex = 0;
    }

    // Remove an effect from the manager
    void RemoveEffect(unique_ptr<ILEDEffect> & effect) override
    {
        if (!effect)
            throw invalid_argument("Cannot remove a null effect.");

        auto it = remove(_effects.begin(), _effects.end(), effect);
        if (it != _effects.end())
        {
            auto index = distance(_effects.begin(), it);
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
    void StartCurrentEffect(ICanvas& canvas) override
    {
        if (IsEffectSelected())
            _effects[_currentEffectIndex]->Start(canvas);
    }

    void SetCurrentEffect(size_t index, ICanvas& canvas) override
    {
        if (index >= _effects.size())
            throw out_of_range("Effect index out of range.");

        _currentEffectIndex = index;

        StartCurrentEffect(canvas);
   }

    // Update the current effect and render it to the canvas
    void UpdateCurrentEffect(ICanvas& canvas, milliseconds millisDelta) override
    {
        if (IsEffectSelected())
            _effects[_currentEffectIndex]->Update(canvas, millisDelta);
    }

    // Switch to the next effect
    void NextEffect() override
    {
        if (!_effects.empty())
            _currentEffectIndex = (_currentEffectIndex + 1) % _effects.size();
    }

    // Switch to the previous effect
    void PreviousEffect() override
    {
        if (!_effects.empty())
            _currentEffectIndex = (_currentEffectIndex == 0) ? _effects.size() - 1 : _currentEffectIndex - 1;
    }

    // Get the name of the current effect
    string CurrentEffectName() const override
    {
        if (IsEffectSelected())
            return _effects[_currentEffectIndex]->Name();
        return "No Effect Selected";
    }

    // Clear all effects

    void ClearEffects() override
    {
        _effects.clear();
        _currentEffectIndex = -1;
    }

    // Start the worker thread to update effects
    
    void Start(ICanvas& canvas) override
    {
        logger->debug("Starting effects manager with {} effects at {} FPS", _effects.size(), _fps);

        if (_running.exchange(true))
            return; // Already running

        _workerThread = thread([this, &canvas]() 
        {
            auto frameDuration = 1000ms / _fps; // Target duration per frame
            auto nextFrameTime = steady_clock::now();
            constexpr auto bUseCompression = true;

            StartCurrentEffect(canvas);

            while (_running)
            {
                // Update the effects and enqueue frames
                UpdateCurrentEffect(canvas, frameDuration);
                for (const auto &feature : canvas.Features())
                {
                    auto frame = feature.get().GetDataFrame();
                    if (bUseCompression)
                    {
                        auto compressedFrame = feature.get().Socket().CompressFrame(frame);
                        feature.get().Socket().EnqueueFrame(std::move(compressedFrame));
                    }
                    else
                    {
                        feature.get().Socket().EnqueueFrame(std::move(frame));
                    }
                }

                // We wait here while periodically checking _running
                
                auto now = steady_clock::now();
                while (now < nextFrameTime && _running) {
                    this_thread::sleep_for(min(steady_clock::duration(10ms), nextFrameTime - now));
                    now = steady_clock::now(); // Update 'now' to avoid an infinite loop
                }

                // Set the next frame target
                nextFrameTime += frameDuration;
            }
        });
    }

    // Stop the worker thread
    void Stop() override
    {
        logger->debug("Stopping effects manager");
        if (!_running.exchange(false))
            return; // Not running

        if (_workerThread.joinable())
            _workerThread.join();
    }

    void SetEffects(vector<unique_ptr<ILEDEffect>> effects) override 
    {
        _effects = std::move(effects);
    }

    void SetCurrentEffectIndex(int index) override 
    {
        _currentEffectIndex = index;
    }

private:
    bool IsEffectSelected() const
    {
        return _currentEffectIndex >= 0 && _currentEffectIndex < static_cast<int>(_effects.size());
    }

    friend void to_json(nlohmann::json& j, const EffectsManager& manager);
    friend void from_json(const nlohmann::json& j, EffectsManager& manager);
};


inline void to_json(nlohmann::json& j, const ILEDEffect& effect) 
{
    // Attempt to dynamically cast to each known concrete type and use its to_json handler
    
    if (const auto* colorWave = dynamic_cast<const ColorWaveEffect*>(&effect)) 
        to_json(j, *colorWave);
    else if (const auto* fireworks = dynamic_cast<const FireworksEffect*>(&effect))
        to_json(j, *fireworks);
    else if (const auto* solidColor = dynamic_cast<const SolidColorFill*>(&effect))
        to_json(j, *solidColor);
    else if (const auto* palette = dynamic_cast<const PaletteEffect*>(&effect))
        to_json(j, *palette);
    else if (const auto* starfield = dynamic_cast<const StarfieldEffect*>(&effect))
        to_json(j, *starfield);
    else if (const auto* video = dynamic_cast<const MP4PlaybackEffect*>(&effect))
        to_json(j, *video);
    else
        throw std::runtime_error("Unknown effect type for serialization: " + string(typeid(effect).name()));
}

// Create an effect from JSON and return a unique pointer to it

template<typename T>
unique_ptr<ILEDEffect> makeFromJson(const nlohmann::json& j) {
    unique_ptr<T> temp;
    from_json(j, temp);
    return temp;
}

// Dynamically deserialize an effect from JSON based on its indicated type 
// and return it on the unique pointer out reference

inline void from_json(const nlohmann::json& j, unique_ptr<ILEDEffect>& effect) 
{
    static const std::unordered_map<std::string, std::unique_ptr<ILEDEffect>(*)(const nlohmann::json&)> effects_map = 
    {
        {ColorWaveEffect::EffectTypeName(),   makeFromJson<ColorWaveEffect>},
        {FireworksEffect::EffectTypeName(),   makeFromJson<FireworksEffect>},
        {SolidColorFill::EffectTypeName(),    makeFromJson<SolidColorFill>},
        {PaletteEffect::EffectTypeName(),     makeFromJson<PaletteEffect>},
        {StarfieldEffect::EffectTypeName(),   makeFromJson<StarfieldEffect>},
        {MP4PlaybackEffect::EffectTypeName(), makeFromJson<MP4PlaybackEffect>}
    };
    auto it = effects_map.find(j["type"]);
    if (it == effects_map.end())
        throw runtime_error("Unknown effect type for deserialization: " + j["type"].get<string>());       
    effect = std::move(it->second(j));
}

inline void to_json(nlohmann::json& j, const IEffectsManager& manager) 
{
    j = {
        {"type", "EffectsManager"},
        {"fps", manager.GetFPS()},
        {"currentEffectIndex", manager.GetCurrentEffect()},
        {"effects", manager.Effects()}
    };
}

inline void from_json(const nlohmann::json& j, IEffectsManager& manager) 
{
    manager.SetFPS(j.at("fps").get<uint16_t>());
    vector<unique_ptr<ILEDEffect>> effects = j.at("effects").get<vector<unique_ptr<ILEDEffect>>>();
    manager.SetEffects(std::move(effects));
}