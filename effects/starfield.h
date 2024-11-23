#pragma once

using namespace std;
using namespace std::chrono;

#include "../interfaces.h"
#include "../ledeffectbase.h"
#include "../pixeltypes.h"
#include <vector>
#include <random>
#include <cmath>

class StarfieldEffect : public LEDEffectBase
{
private:
    struct Star
    {
        double x, y;        // Current position
        double dx, dy;      // Velocity components
        uint8_t brightness; // Star brightness
    };

    std::vector<Star> _stars; // Active stars
    int _starCount;           // Number of stars
    std::mt19937 _rng;        // Random number generator
    std::uniform_real_distribution<double> _speedDist;      // Speed distribution
    std::uniform_real_distribution<double> _directionDist;  // Direction distribution
    std::uniform_int_distribution<uint8_t> _brightnessDist; // Brightness distribution

    int _centerX, _centerY; // Center of the canvas

public:
    StarfieldEffect(const std::string& name, int starCount = 100)
        : LEDEffectBase(name), _starCount(starCount), _rng(std::random_device{}()),
          _speedDist(5.0, 20.0), // Increased speed for hyperspace effect
          _directionDist(0, 2 * M_PI), // Full 360° angular range
          _brightnessDist(192, 255), _centerX(0), _centerY(0)
    {
    }

    void Start(ICanvas& canvas) override
    {
        _centerX = canvas.Graphics().Width() / 2;
        _centerY = canvas.Graphics().Height() / 2;

        _stars.clear();
        for (int i = 0; i < _starCount; ++i)
        {
            _stars.push_back(CreateRandomStar());
        }
    }

    void Update(ICanvas& canvas, milliseconds deltaTime) override
    {
        auto& graphics = canvas.Graphics();
        graphics.Clear(CRGB::Black); // Clear the canvas

        double timeFactor = deltaTime.count() / 1000.0; // Convert delta time to seconds

        for (auto& star : _stars)
        {
            // Update position based on velocity and time
            const auto xScale = (double) (graphics.Width() / graphics.Height()) / 2.0;
            star.x += star.dx * timeFactor * xScale;
            star.y += star.dy * timeFactor;

            // If the star is out of bounds, respawn it
            if (star.x < 0 || star.x >= graphics.Width() || star.y < 0 || star.y >= graphics.Height())
            {
                star = CreateRandomStar();
            }

            // Draw the star
            int ix = static_cast<int>(star.x);
            int iy = static_cast<int>(star.y);
            graphics.SetPixel(ix, iy, CRGB(star.brightness, star.brightness, star.brightness));
        }
    }

private:
    Star CreateRandomStar()
    {
        // Generate a random speed and direction
        double speed = _speedDist(_rng);
        double angle = _directionDist(_rng);

        // Compute velocity components
        double dx = speed * cos(angle);
        double dy = speed * sin(angle);

        return Star{static_cast<double>(_centerX), static_cast<double>(_centerY), dx, dy, _brightnessDist(_rng)};
    }
};
