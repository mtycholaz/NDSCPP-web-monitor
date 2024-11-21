## Interfaces Overview

### ISocketChannel  
Defines a communication protocol for managing socket connections and sending data to a server.  
Provides methods for enqueuing frames, retrieving connection status, and tracking performance metrics.

### ICanvas  
Represents a 2D drawing surface that manages LED features and provides rendering capabilities.  
Can contain multiple `ILEDFeature` instances, with features mapped to specific regions of the canvas.

### ILEDGraphics  
Provides drawing primitives for 1D and 2D LED features, such as lines, rectangles, gradients, and circles.  
Exposes APIs for pixel manipulation and advanced rendering techniques.

### ILEDEffect  
Defines lifecycle hooks (`Start` and `Update`) for applying visual effects on LED canvases.  
Encourages modular effect design, allowing dynamic assignment and switching of effects.

### ILEDFeature  
Represents a 2D collection of LEDs with positioning, rendering, and configuration capabilities.  
Provides APIs for interacting with its parent canvas and retrieving its assigned color data.

### ISocketController  
Manages a collection of `ISocketChannel` instances, providing lifecycle control and connection pooling.  
Handles adding, removing, and retrieving channels by hostname.

## Classes Overview

### SocketChannel  
Implements `ISocketChannel` to manage socket connections and transmit LED frame data.  
Includes support for data compression and efficient queuing of frames.  
Tracks connection state and throughput metrics.

### SocketController  
Implements `ISocketController` to manage a collection of `SocketChannel` instances, providing start, stop, and connection management.  
Ensures thread-safe operations with internal locking mechanisms.

### Canvas  
Implements `ICanvas` and `ILEDGraphics`, representing a 2D drawing surface with support for multiple LED features.  
Features advanced rendering capabilities, including drawing primitives, gradients, and solid fills.  
Serves as the primary interface for rendering effects to assigned LED features.

### LEDFeature  
Implements `ILEDFeature` to represent a logical set of LEDs within a canvas.  
Handles retrieving pixel data from its assigned region of the parent canvas for transmission over a socket.  
Includes attributes such as offset, dimensions, and channel assignment.

### EffectsManager  
Manages a collection of effects and controls the currently active effect.  
Applies the active effect to an `ICanvas` instance during rendering.  
Provides utilities for switching between effects (`NextEffect` and `PreviousEffect`).

### WebServer  
Hosts a REST API for interacting with and controlling LED canvases and their features.  
Supports dynamic management of features, canvases, and effects via HTTP endpoints.

### Utilities  
Provides static helper functions for byte manipulation, color conversion, and data combination tasks.  
Includes compression utilities (using zlib), endian-safe conversions, and drawing utilities for LED data.

### CRGB  
Represents a 24-bit RGB color, including utility methods for HSV-to-RGB conversion and brightness adjustment.  
Forms the base unit of color manipulation across the system.

```text -----------------------------------          -------------------------------
|          ISocketChannel           |        |           ICanvas             |
|-----------------------------------|        |-------------------------------|
| + HostName()                      |        | + Width()                     |
| + FriendlyName()                  |        | + Height()                    |
| + Width()                         |        | + AddFeature()                |
| + Height()                        |        | + RemoveFeature()             |
| + Port()                          |        | + DrawPixel()                 |
| + EnqueueFrame()                  |        | + FillSolid()                 |
| + IsConnected()                   |        | + FillRainbow()               |
| + BytesPerSecond()                |        | + Blur()                      |
| + ResetBytesPerSecond()           |        | + GetPixel()                  |
| + Start()                         |        | + Update()                    |
| + Stop()                          |        --------------------------------|
-------------------------------------                                      

------------------------------------        ---------------------------------
|          ILEDGraphics            |        |          ILEDFeature          |
|----------------------------------|        |-------------------------------|
| + DrawPixel()                    |        | + Width()                     |
| + DrawLine()                     |        | + Height()                    |
| + DrawRect()                     |        | + Offset()                    |
| + DrawCircle()                   |        | + HostName()                  |
| + FillSolid()                    |        | + FriendlyName()              |
| + FillGradient()                 |        | + RedGreenSwap()              |
| + GetPixel()                     |        | + Channel()                   |
------------------------------------        ---------------------------------

Classes:
------------------------------------         -------------------------------
|           SocketChannel          |<-------|       SocketController        |
|----------------------------------|        |-------------------------------|
| - _hostName                      |        | - _channels                   |
| - _friendlyName                  |        | - _mutex                      |
| - _width                         |        |-------------------------------|
| - _height                        |        | + AddChannel()                |
| - _port                          |        | + RemoveChannel()             |
| - _mutex                         |        | + StartAll()                  |
| - _isConnected                   |        | + StopAll()                   |
|----------------------------------|        | + TotalBytesPerSecond()       |
| + Implements: ISocketChannel     |        |-------------------------------|
-----------------------------------

-----------------------------------          ---------------------------------
|              Canvas               |<-------|           Feature              |
|-----------------------------------|        |-=------------------------------|
| - _width                          |        | - _offsetX                     |
| - _height                         |        | - _offsetY                     |
| - _features                       |        | - _hostName                    |
| - _graphicsEngine                 |        | - _channel                     |
|-----------------------------------|        | - _redGreenSwap                |
| + Implements: ICanvas, ILEDGraphics        |--------------------------------|
------------------------------------|        | + Implements: ILEDFeature      |
                                       ----------------------------------------

```
