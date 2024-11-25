# NightDriver Server - Overview

## What it is

It delivers WiFI packets of color data to ESP32 chips that show that color data on LED strips or matrices connected to them.  Here's an example:

My house has a long run of 8000 LEDs, like Christmas lights.  Since each ESP32 running NightDriverStrip can only refresh about 1000 LEDs at 30fps, I have broken the run into 8 individual ESP32s, each connected to 1000 LEDs.  They could each run the same effect, but it would be hard to sync, and effects could not span across strips.

NightDriverServer instead composes the drawing on a larger Canvas object, in this case 8000 pixels wide.  Each ESP32 is represented as an LEDFeature object, 1000 pixels wide.  The first is at offset 0 in the canvas, the next at 1000, then 2000, and so on.  

30 times per second, NDSCPP renders the scene to the 8000-pixel canvas.  Worker threads then split that up into eight separate chunks of 1000 pixels and send each as a packet to the appropriate LED strip, and they all act in concert as one long strip.

Each NightDriverStrip has a socket available on port 49152 to receive frames of color data, normally at up to 60fps.  The strips buffer a few seconds' worth of frames internally and display them perfectly synced by SNTP time, so an effect frame that is supposed to appear all at once across the strips still does so despite delays in Wi-Fi and so on.

It allows you to build a much larger scene from many little ESP32 installs and control it via WiFi.  

Imagine you had a restaurant with 10 tables.  Each table has an LED candle with 16 LEDs.  There are two ways to configure this.  If you want each candle to do the same thing, you would make a Canvas that is 16 pixels long and then place 10 LEDFeatures in it, all at offset 0.

If you wanted each candle to render differently, you could make a Canvas that is 160 pixels long and offset the LEDFeatures at 0, 10, 20, 30, and so on.  Then your drawing effect could draw to individual candles.

Alternatively, you could also define 10 Canvases of 16 pixels, and each Canvas has one LEDFeature of 10 pixels each, and each candle in that case can have its own effect.

-----
<img width="1774" alt="2024-11-23_21-33-58" src="https://github.com/user-attachments/assets/820e37e6-8e1b-4b4b-ab10-6a2f27a34d7f">

NightDriver Server is a C++ project designed to manage LED displays by organizing them into canvases, applying effects, and transmitting data to remote LED controllers. The code is modular, and leverages interface to separate concerns, making it extensible and straightforward to maintain.

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

This project is dependent on the libraries for asio (because Crow uses it), lpthreads, and lz.  You will also need the json library from homebrew (at least on the Mac):
brew install asio
brew install nlohmann-json

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
### ICanvas  
Represents a 2D drawing surface that manages LED features and provides rendering capabilities.  
Can contain multiple `ILEDFeature` instances, with features mapped to specific regions of the canvas.

### ILEDGraphics  
Provides drawing primitives for 1D and 2D LED features, such as lines, rectangles, gradients, and circles.  
Exposes APIs for pixel manipulation and advanced rendering techniques.

### ILEDEffect  
Defines lifecycle hooks (`Start` and `Update`) for applying visual effects on LED canvases.  
Encourages modular effect design, allowing dynamic assignment and switching of effects.
### ILEDEffect  
Defines lifecycle hooks (`Start` and `Update`) for applying visual effects on LED canvases.  
Encourages modular effect design, allowing dynamic assignment and switching of effects.

### IEffectManager
Each canvas has an EffectsManager that does the actual drawing of effects to it, and that EffectsManager
manages a set of ILEDEffect objects.
### IEffectManager
Each canvas has an EffectsManager that does the actual drawing of effects to it, and that EffectsManager
manages a set of ILEDEffect objects.

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

```text

-------------------------------------        ---------------------------------
-------------------------------------        ---------------------------------
|          ISocketChannel           |        |           ICanvas             |
|-----------------------------------|        |-------------------------------|
| + HostName()                      |        | + Width()                     |
| + FriendlyName()                  |        | + Height()                    |
| + Width()                         |        | + AddFeature()                |
| + Height()                        |        | + RemoveFeature()             |
| + Port()                          |        | + Graphics()                  |
| + EnqueueFrame()                  |        | + Effects()                   |
| + IsConnected()                   |        | + Update()                    |
| + BytesPerSecond()                |        |-------------------------------|
| + ResetBytesPerSecond()           |
| + Start()                         |
| + Stop()                          |
-------------------------------------
| + Port()                          |        | + Graphics()                  |
| + EnqueueFrame()                  |        | + Effects()                   |
| + IsConnected()                   |        | + Update()                    |
| + BytesPerSecond()                |        |-------------------------------|
| + ResetBytesPerSecond()           |
| + Start()                         |
| + Stop()                          |
-------------------------------------

------------------------------------        ---------------------------------
|          ILEDGraphics            |        |          ILEDFeature          |
|----------------------------------|        |-------------------------------|
| + DrawPixel()                    |        | + Width()                     |
| + DrawLine()                     |        | + Height()                    |
| + DrawRect()                     |        | + OffsetX()                   |
| + DrawCircle()                   |        | + OffsetY()                   |
| + FillSolid()                    |        | + HostName()                  |
| + FillGradient()                 |        | + FriendlyName()              |
| + GetPixel()                     |        | + RedGreenSwap()              |
------------------------------------        | + Channel()                   |
                                            | + BatchSize()                 |
                                            |-------------------------------|
| + DrawRect()                     |        | + OffsetX()                   |
| + DrawCircle()                   |        | + OffsetY()                   |
| + FillSolid()                    |        | + HostName()                  |
| + FillGradient()                 |        | + FriendlyName()              |
| + GetPixel()                     |        | + RedGreenSwap()              |
------------------------------------        | + Channel()                   |
                                            | + BatchSize()                 |
                                            |-------------------------------|

Classes:
------------------------------------         -------------------------------
|           SocketChannel          |<-------|       SocketController        |
|----------------------------------|        |-------------------------------|
|           SocketChannel          |<-------|       SocketController        |
|----------------------------------|        |-------------------------------|
| - _hostName                      |        | - _channels                   |
| - _friendlyName                  |        | - _mutex                      |
| - _port                          |        |-------------------------------|
| - _mutex                         |        | + AddChannel()                |
| - _isConnected                   |        | + RemoveChannel()             |
| - _bytesPerSecond                |        | + RemoveAllChannels()         |
|----------------------------------|        | + StartAll()                  |
| + Implements: ISocketChannel     |        | + StopAll()                   |
------------------------------------        | + FindChannelByHost()         |
                                             --------------------------------
| - _port                          |        |-------------------------------|
| - _mutex                         |        | + AddChannel()                |
| - _isConnected                   |        | + RemoveChannel()             |
| - _bytesPerSecond                |        | + RemoveAllChannels()         |
|----------------------------------|        | + StartAll()                  |
| + Implements: ISocketChannel     |        | + StopAll()                   |
------------------------------------        | + FindChannelByHost()         |
                                             --------------------------------

-----------------------------------          ---------------------------------
|              Canvas               |<-------|           Feature             |
|-----------------------------------|        |-------------------------------|
| - _width                          |        | - _offsetX                    |
| - _height                         |        | - _offsetY                    |
| - _features                       |        | - _hostName                   |
| - _graphics                       |        | - _channel                    |
| - _effects                        |        | - _redGreenSwap               |
|-----------------------------------|        | - _batchSize                  |
| + Implements: ICanvas, ILEDGraphics        |-------------------------------|
|-----------------------------------         | + Implements: ILEDFeature     |
                                             ---------------------------------

-----------------------------------
|          EffectsManager          |
|----------------------------------|
| - _effects                       |
| - _currentEffectIndex            |
| - _workerThread                  |
| - _running                       |
|----------------------------------|
| + AddEffect()                    |
| + RemoveEffect()                 |
| + Start()                        |
| + Stop()                         |
| + NextEffect()                   |
| + PreviousEffect()               |
| + UpdateCurrentEffect()          |
| + SetCurrentEffect()             |
| + CurrentEffectName()            |
| + ClearEffects()                 |
-----------------------------------