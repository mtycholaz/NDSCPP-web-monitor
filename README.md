# NightDriver Server - Overview

NightDriver Server is a C++ project designed to manage LED displays by organizing them into canvases, applying effects, and transmitting data to remote LED controllers. The code is modular and leverages interfaces to separate concerns, making it extensible and straightforward to maintain.

Key concepts for programmers:

- **Canvas**: Represents a 2D grid of LEDs where drawing operations and effects are applied. A canvas can contain multiple LED features.
- **LEDFeature**: A specific section of the canvas, associated with a remote LED controller. Each feature defines properties such as dimensions, offsets, and communication parameters.
- **SocketChannel**: Manages the network connection to a remote LED controller, transmitting the pixel data for a feature.
- **Effects**: Visual animations or patterns applied to a canvas, implemented using the `ILEDEffect` interface.
- **Utilities**: Contains helper functions for tasks like byte manipulation, data compression, and color conversion.

### Getting Started
1. **Define Features**: Create LED features, specifying dimensions, offsets, and the associated socket connections.
2. **Configure a Canvas**: Combine one or more features into a canvas to organize the drawing surface.
3. **Apply Effects**: Use the `EffectsManager` to apply visual effects to the canvas.
4. **Transmit Data**: Socket channels handle sending the rendered canvas data to the remote LED controllers.

The project includes a REST API via the `WebServer` class to control and configure canvases dynamically. For detailed interface descriptions and class diagrams, refer to the sections below.

This repository is designed for programmers familiar with modern C++ (C++20 and later) and concepts like interfaces, threading, and network communication. Jump into the code, and start by exploring the interfaces and their implementing classes to understand the system's structure.

This project is dependent on the libraries for libmicrohttpd, lpthreads, and lz.

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
