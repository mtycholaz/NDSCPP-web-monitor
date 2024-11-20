#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <map>

// ILEDGraphics interface for 2D drawing primitives

class ILEDGraphics
{
public:
    virtual ~ILEDGraphics() = default;

    // Accessors for individual pixels
    virtual CRGB GetPixel(uint32_t x, uint32_t y) const = 0;
    virtual void DrawPixel(uint32_t x, uint32_t y, CRGB color) = 0;

    // Drawing methods
    virtual void DrawPixels(double position, double count, CRGB color) = 0;
    virtual void DrawLine(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, CRGB color) = 0;
    virtual void DrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, CRGB color) = 0;
    virtual void DrawCircle(uint32_t x0, uint32_t y0, uint32_t radius, CRGB color) = 0;

    // Fill and effect methods
    virtual void FillSolid(CRGB color) = 0;
    virtual void FillRainbow(double startHue, double deltaHue) = 0;
};

// ILEDFeature interface represents a 2D set of CRGB objects
class ILEDFeature
{
public:
    virtual ~ILEDFeature() = default;

    // Accessor methods
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;
    virtual const std::string &HostName() const = 0;
    virtual const std::string &FriendlyName() const = 0;
    virtual uint32_t OffsetX() const = 0;
    virtual uint32_t OffsetY() const = 0;
    virtual bool Reversed() const = 0;
    virtual uint8_t Channel() const = 0;
    virtual bool RedGreenSwap() const = 0;
    virtual uint32_t BatchSize() const = 0;

    // Data retrieval
    virtual std::vector<uint8_t> GetPixelData() const = 0;

    // State handling
    virtual void Clear() = 0; // Resets the feature state
};

class ICanvas : public ILEDGraphics
{
public:
    virtual ~ICanvas() = default;

    // Accessors for canvas dimensions
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;

    // Accessors for features
    virtual std::vector<std::shared_ptr<ILEDFeature>>& Features() = 0;
    virtual const std::vector<std::shared_ptr<ILEDFeature>>& Features() const = 0;

    // Add or remove features
    virtual void AddFeature(std::shared_ptr<ILEDFeature> feature) = 0;
    virtual void RemoveFeature(std::shared_ptr<ILEDFeature> feature) = 0;
};



// ILEDEffect interface for effects applied to LED features or canvases
class ILEDEffect
{
public:
    virtual ~ILEDEffect() = default;

    // Get the name of the effect
    virtual const std::string& Name() const = 0;

    // Called when the effect starts
    virtual void Start(ICanvas& canvas) = 0;

    // Called to update the effect, given a canvas and timestamp
    virtual void Update(ICanvas& canvas, std::chrono::milliseconds deltaTime) = 0;

};
class ISocketChannel
{
public:
    virtual ~ISocketChannel() = default;

    // Accessors for channel details
    virtual const std::string& HostName() const = 0;
    virtual const std::string& FriendlyName() const = 0;
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;
    virtual uint16_t Port() const = 0;

    // Data transfer methods
    virtual bool EnqueueFrame(const std::vector<uint8_t>& frameData, std::chrono::time_point<std::chrono::system_clock> timestamp) = 0;

    // Connection status
    virtual bool IsConnected() const = 0;

    // Performance tracking
    virtual uint32_t BytesPerSecond() const = 0;
    virtual void ResetBytesPerSecond() = 0;

    // Start and stop operations
    virtual void Start() = 0;
    virtual void Stop() = 0;
};