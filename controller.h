#pragma once
using namespace std;

// Controller
//
// Represents the entire system, contains the canvases which contain the features which contain the effects.

#include "json.hpp"
#include "interfaces.h"
#include "basegraphics.h"
#include "effectsmanager.h"
#include <vector>
#include "global.h"
#include "canvas.h"
#include "ledfeature.h"
#include "serialization.h"
#include "effects/colorwaveeffect.h"
#include "effects/starfield.h"
#include "effects/videoeffect.h"
#include "effects/misceffects.h"
#include "palette.h"
#include "effects/paletteeffect.h"
#include "effects/fireworkseffect.h"

class Controller;
inline void from_json(const nlohmann::json &j, unique_ptr<Controller> & ptrController);

class Controller : public IController
{
  private:

    vector<unique_ptr<ICanvas>> _canvases;
    uint16_t                    _port;

  public:

    Controller(uint16_t port = 7777) : _port(port)
    {
    }

    Controller() : _port(7777) 
    {
    }

    vector<reference_wrapper<ICanvas>> Canvases() const override
    {
        vector<std::reference_wrapper<ICanvas>> canvases;
        canvases.reserve(_canvases.size());
        for (const auto& canvas : _canvases)
            canvases.push_back(*canvas);
        return canvases;
    }

    static std::unique_ptr<Controller> CreateFromFile(const std::string& filePath) 
    {
        // Open the file and parse the JSON
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open file: " + filePath);
        }

        nlohmann::json jsonData;
        file >> jsonData;

        // Deserialize the JSON into a unique_ptr<Controller>
        std::unique_ptr<Controller> ptrController;
        from_json(jsonData, ptrController);  // Explicitly use your from_json function

        return ptrController;
    }




    uint16_t GetPort() const override
    {
        return _port;
    }

    void SetPort(uint16_t port) override
    {
        _port = port;
    }

    bool AddFeatureToCanvas(uint16_t canvasId, unique_ptr<ILEDFeature> feature) override
    {
        logger->debug("Adding feature to canvas {}...", canvasId);
        GetCanvasById(canvasId).AddFeature(std::move(feature));
        return true;
    }

    void RemoveFeatureFromCanvas(uint16_t canvasId, uint16_t featureId) override
    {
        logger->debug("Removing feature {} from canvas {}...", featureId, canvasId);
        GetCanvasById(canvasId).RemoveFeatureById(featureId);
    }

    void Connect() override
    {
        logger->debug("Connecting canvases...");

        for (const auto &canvas : _canvases)
            for (const auto &feature : canvas->Features())
                feature.get().Socket().Start();
    }

    void Disconnect() override
    {
        logger->debug("Disconnecting canvases...");

        for (const auto &canvas : _canvases)
            for (const auto &feature : canvas->Features())
                feature.get().Socket().Stop();
    }

    void Start() override
    {
        logger->debug("Starting canvases...");

        for (auto &canvas : _canvases)
            canvas->Effects().Start(*canvas);
    }

    void Stop() override
    {
        logger->debug("Stopping canvases...");

        for (auto &canvas : _canvases)
            canvas->Effects().Stop();
    }

    bool AddCanvas(unique_ptr<ICanvas> ptrCanvas) override
    {
        logger->debug("Adding canvas {}...", ptrCanvas->Name());

        // This is a bit odd; we try get the current canvas with the ID specified by the new one,
        // and we only proceed in the exception case if the canvas doesn't exist, where we add it

        try
        {
            GetCanvasById(ptrCanvas->Id());
            logger->error("Canvas with ID {} already exists.", ptrCanvas->Id());
            return false;
        }
        catch(const out_of_range &)               
        {
            _canvases.push_back(std::move(ptrCanvas));    
            return true;
        }
    }

    bool DeleteCanvasById(uint32_t id) override
    {
        logger->debug("Deleting canvas {}...", id);

        for (size_t i = 0; i < _canvases.size(); ++i)
        {
            if (_canvases[i]->Id() == id)
            {
                _canvases.erase(_canvases.begin() + i);
                return true;
            }
        }

        logger->error("Canvas with ID {} not found in DeleteCanvasById.", id);
        return false;
    }

    bool UpdateCanvas(unique_ptr<ICanvas> ptrCanvas) override
    {
        logger->debug("Updating canvas {}...", ptrCanvas->Name());

        // Find the existing canvas with the matching Id and replace it
        for (size_t i = 0; i < _canvases.size(); ++i)
        {
            if (_canvases[i]->Id() == ptrCanvas->Id())
            {
                _canvases[i] = std::move(ptrCanvas);
                return true;
            }
        }

        logger->error("Canvas with ID {} not found in UpdateCanvas.", ptrCanvas->Id());
        return false;
    }

    ICanvas & GetCanvasById(uint16_t id) const override
    {
        // Find and return the canvas with the matching ID
        for (auto &canvas : _canvases)
            if (canvas->Id() == id)
                return *canvas.get();

        logger->debug("Canvas with ID {} not found in GetCanvasById.", id);
        throw out_of_range("Canvas not found");
    }

    vector<reference_wrapper<ISocketChannel>> GetSockets() const override
    {
        vector<reference_wrapper<ISocketChannel>> sockets;
        for (const auto &canvas : _canvases)
            for (const auto &feature : canvas->Features())
                sockets.push_back(feature.get().Socket());
        return sockets;
    }

    const ISocketChannel & GetSocketById(uint16_t id) const override
    {
        for (auto &canvas : _canvases)
            for (auto &feature : canvas->Features())
                if (feature.get().Socket().Id() == id)
                    return feature.get().Socket();

        logger->error("Socket with ID {} not found in GetSocketById.", id);                     
        throw out_of_range("Socket not found by id");
    }
};

