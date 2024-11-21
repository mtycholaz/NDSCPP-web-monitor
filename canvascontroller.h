#pragma once

// Not used yet 

#include <vector>
#include <memory>
#include <mutex>
#include "canvas.h"

class CanvasController
{
public:
    CanvasController() = default;
    ~CanvasController() = default;

    // Add or remove a canvas
    void AddCanvas(std::shared_ptr<ICanvas> canvas);
    void RemoveCanvas(const std::string& name);

    // Retrieve a canvas by name
    std::shared_ptr<ICanvas> GetCanvas(const std::string& name);

    // Start, stop, and update all canvases
    void StartAll();
    void StopAll();
    void UpdateAll();

private:
    std::vector<std::shared_ptr<ICanvas>> _canvases;
    std::mutex _mutex;
};
