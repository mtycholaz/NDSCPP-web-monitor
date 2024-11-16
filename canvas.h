// Canvas.h
//
// A canvas represents a 2D grid of pixels that can be drawn to and displayed.  It is comprised 
// of attributes like overall width and height in pixels, and a collection of pixel colors.  The
// pixel colors are rendered by a worker thread that periodically updates the pixel colors by 
// calling the render methods of the individual strips that comprise the canvas. 
//
// So, if you had a strip that was made up of four 1000-pixel strips, it would be a 4000x1 canvas
// and it would have for LEDFeature objects, each of which would be responsible for rendering its
// own portion of the canvas.


#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <iostream>

class Canvas
{
protected:
    std::string Name;
    uint32_t Width;
    uint32_t Height;
    std::vector<CRGB> LEDs;
    std::atomic<bool> Enabled{true};
    std::atomic<uint32_t> SpareTime{1000};
    std::mutex LEDMutex;
    std::thread WorkerThread;

public:
    Canvas(const std::string &name, uint32_t width, uint32_t height, bool enabled)
        : Name(name), Width(width), Height(height), Enabled(enabled)
    {
        LEDs.resize(Width * Height, CRGB::Black);
    }

    virtual ~Canvas()
    {
        StopWorkerThread();
    }

    void StartWorkerThread()
    {
        if (!WorkerThread.joinable())
        {
            Enabled.store(true);
            WorkerThread = std::thread(&Canvas::WorkerDrawAndSendLoop, this);
        }
    }

    void StopWorkerThread()
    {
        if (WorkerThread.joinable())
        {
            Enabled.store(false);  // Signal the thread to exit
            WorkerThread.join();   // Wait for the thread to finish
        }
    }

    uint32_t GetPixelIndex(uint32_t x, uint32_t y) const
    {
        return (y * Width) + x;
    }

    void SetPixel(uint32_t x, uint32_t y, const CRGB &color)
    {
        std::lock_guard<std::mutex> lock(LEDMutex);
        if (x < Width && y < Height)
            LEDs[GetPixelIndex(x, y)] = color;
    }

    CRGB GetPixel(uint32_t x, uint32_t y) const
    {
        if (x < Width && y < Height)
            return LEDs[GetPixelIndex(x, y)];
        return CRGB::Black;
    }

protected:
    void WorkerDrawAndSendLoop()
    {
        while (Enabled.load())
        {
            {
                std::lock_guard<std::mutex> lock(LEDMutex);
                // Example drawing logic: Set all pixels to blue
                for (auto &pixel : LEDs)
                    pixel = CRGB(0, 0, 255);
            }

            // Simulate a frame rate of 30 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
};
