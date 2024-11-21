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

This repository is designed for programmers familiar with modern C++ (C++17 and later) and concepts like interfaces, threading, and network communication. Jump into the code, and start by exploring the interfaces and their implementing classes to understand the system's structure.

## Interfaces Overview

### ISocketChannel  
Defines a communication protocol for managing socket connections and sending data to a server.

### ICanvas  
Represents a 2D drawing surface that manages LED features and provides rendering capabilities.

### ILEDGraphics  
Provides drawing primitives for 1D and 2D LED features, such as lines, rectangles, and gradients.

### ILEDEffect  
Defines lifecycle hooks (Start and Update) for applying visual effects on LED canvases.

### ILEDFeature  
Represents a 2D collection of LEDs with positioning, rendering, and configuration capabilities.

### ISocketController  
Manages a collection of socket channels, providing lifecycle control and connection pooling.

## Classes Overview

### SocketChannel  
Implements ISocketChannel to manage socket connections and transmit LED frame data.

### SocketController  
Implements ISocketController to manage a collection of SocketChannel instances, providing start, stop, and connection management.

### Canvas  
Implements ICanvas and ILEDGraphics, representing a 2D drawing surface with support for multiple LED features.

### WebServer  
Hosts a REST API for interacting with and controlling LED canvases and their features.

### Utilities  
Provides static helper functions for byte manipulation, color conversion, and data combination tasks.

### CRGB  
Represents a 24-bit RGB color, including utility methods for HSV-to-RGB conversion and brightness adjustment.

```text
 -----------------------------------          -------------------------------
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
| - _width                          |        | - _offset                      |
| - _height                         |        | - _hostName                    |
| - _features                       |        | - _channel                     |
| - _graphicsEngine                 |        | - _redGreenSwap                |
|-----------------------------------|        |--------------------------------|
| + Implements: ICanvas, ILEDGraphics        | + Implements: ILEDFeature      |
------------------------------------|        ---------------------------------
```
