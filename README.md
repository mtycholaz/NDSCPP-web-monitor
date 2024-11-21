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
