#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "crow_all.h"
#include "interfaces.h" // Assuming ICanvas is defined here

using namespace std;

class WebServer
{
private:
    const vector<unique_ptr<ICanvas>> &_allCanvases; // Reference to all canvases
    crow::SimpleApp app;

public:
    WebServer(const vector<unique_ptr<ICanvas>> &allCanvases) : _allCanvases(allCanvases)
    {
    }

    ~WebServer()
    {
    }

    void Start()
    {
        std::cout << "Starting WebServer...\n";
        std::cout << "Address of this: " << this << std::endl;
        std::cout << "Number of canvases: " << _allCanvases.size() << std::endl;

        // Define the `/api/canvases` endpoint
        CROW_ROUTE(app, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([this]()
            {
                auto canvasesJson = nlohmann::json::array();
                for (size_t i = 0; i < _allCanvases.size(); ++i)
                {
                    if (_allCanvases[i]) // Ensure the canvas pointer is valid
                    {
                        nlohmann::json canvasJson;
                        to_json(canvasJson, *_allCanvases[i]); // Use the utility function
                        canvasJson["id"] = i; // Add ID for reference
                        canvasesJson.push_back(canvasJson);
                    }
                }
                return crow::response{canvasesJson.dump()};
            });

        // Define the `/api/canvases/:id` endpoint
        CROW_ROUTE(app, "/api/canvases/<int>")
            .methods(crow::HTTPMethod::GET)([this](int id)
            {
                if (id < 0 || id >= _allCanvases.size() || !_allCanvases[id])
                    return crow::response(404, R"({"error": "Canvas not found"})");

                nlohmann::json canvasJson;
                to_json(canvasJson, *_allCanvases[id]); // Use the utility function
                canvasJson["id"] = id; // Include ID in the details
                return crow::response{canvasJson.dump()};
            });

        // Start the server
        app.port(7777).multithreaded().run();
    }

    void Stop()
    {
        app.stop();
    }
};
