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

    // Drawing primitives
    virtual void DrawPixel(uint32_t x, uint32_t y, const CRGB& color) = 0;
    virtual void DrawPixel(uint32_t index, const CRGB& color) = 0;
    virtual CRGB GetPixel(uint32_t x, uint32_t y) const = 0;
    virtual CRGB GetPixel(uint32_t index) const = 0;
    virtual void BlendPixel(uint32_t x, uint32_t y, const CRGB& color) = 0;
    
    virtual void DrawFastVLine(uint32_t x, uint32_t y, uint32_t h, const CRGB& color) = 0;
    virtual void DrawFastHLine(uint32_t x, uint32_t y, uint32_t w, const CRGB& color) = 0;
    virtual void DrawRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const CRGB& color) = 0;

    // Filling primitives
    virtual void FillSolid(const CRGB& color) = 0;
    virtual void FillRainbow(double startHue = 0.0, double deltaHue = 5.0) = 0;
};

// ILEDEffect interface for effects applied to LED features or canvases
class ILEDEffect
{
public:
    virtual ~ILEDEffect() = default;

    virtual void Start() = 0; // Called when the effect starts
    virtual void Update() = 0; // Called periodically to update the effect
};

// ILEDFeature interface represents a 2D set of CRGB objects
class ILEDFeature : public ILEDGraphics
{
public:
    virtual ~ILEDFeature() = default;

    virtual const std::string& HostName() const = 0;
    virtual const std::string& FriendlyName() const = 0;
    virtual bool CompressData() const = 0;
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;
    virtual uint32_t Offset() const = 0;
    virtual bool Reversed() const = 0;
    virtual uint8_t Channel() const = 0;
    virtual bool RedGreenSwap() const = 0;
    virtual uint32_t BatchSize() const = 0;
};

class ICanvas
{
public:
    virtual ~ICanvas() = default;

    virtual const std::string& Name() const = 0;
    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;

    virtual void AddFeature(std::shared_ptr<ILEDFeature> feature, uint32_t x, uint32_t y) = 0;
    virtual void RemoveFeature(std::shared_ptr<ILEDFeature> feature) = 0;

    virtual const std::map<std::shared_ptr<ILEDFeature>, std::pair<uint32_t, uint32_t>>& Features() const = 0;
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
    virtual bool EnqueueFrame(const std::vector<CRGB>& frame, std::chrono::time_point<std::chrono::system_clock> timestamp) = 0;

    // Connection status
    virtual bool IsConnected() const = 0;

    // Performance tracking
    virtual uint32_t BytesPerSecond() const = 0;
    virtual void ResetBytesPerSecond() = 0;

    // Start and stop operations
    virtual void Start() = 0;
    virtual void Stop() = 0;
};