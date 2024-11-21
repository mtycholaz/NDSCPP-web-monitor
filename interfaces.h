#pragma once

// Interfaces
// 
// This file contains the interfaces for the various classes in the project.  The interfaces
// are used to define the methods that must be implemented by the classes that use them.  This
// allows the classes to be decoupled from each other, and allows for easier testing and
// maintenance of the code.  It also presumably makes it easier in the future to interop
// with other languages, etc.

#include <string>
#include <vector>
#include <cstdint>
#include <map>

// ILEDGraphics interface for 2D drawing primitives

// Interface ILEDGraphics
class ILEDGraphics 
{
public:
    virtual ~ILEDGraphics() = default;

    virtual uint32_t Width() const = 0;
    virtual uint32_t Height() const = 0;
    virtual void SetPixel(int x, int y, const CRGB& color) = 0;
    virtual CRGB GetPixel(int x, int y) const = 0;
    virtual void Clear(const CRGB& color) = 0;
    virtual void FillRectangle(int x, int y, int width, int height, const CRGB& color) = 0;
    virtual void DrawLine(int x1, int y1, int x2, int y2, const CRGB& color) = 0;
    virtual void DrawCircle(int x, int y, int radius, const CRGB& color) = 0;
    virtual void FillCircle(int x, int y, int radius, const CRGB& color) = 0;
    virtual void DrawRectangle(int x, int y, int width, int height, const CRGB& color) = 0;
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
    virtual std::vector<uint8_t> GetDataFrame() const = 0;    

};

class ICanvas 
{
public:
    virtual ~ICanvas() = default;
    // Accessors for features
    virtual std::vector<std::shared_ptr<ILEDFeature>>& Features() = 0;
    virtual const std::vector<std::shared_ptr<ILEDFeature>>& Features() const = 0;

    // Add or remove features
    virtual void AddFeature(std::shared_ptr<ILEDFeature> feature) = 0;
    virtual void RemoveFeature(std::shared_ptr<ILEDFeature> feature) = 0;

    virtual ILEDGraphics & Graphics() = 0;
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
    virtual uint16_t Port() const = 0;

    // Data transfer methods
    virtual bool EnqueueFrame(const std::vector<uint8_t>& frameData) = 0;
    virtual std::vector<uint8_t> CompressFrame(const std::vector<uint8_t>& data) = 0;

    // Connection status
    virtual bool IsConnected() const = 0;

    // Start and stop operations
    virtual void Start() = 0;
    virtual void Stop() = 0;
};