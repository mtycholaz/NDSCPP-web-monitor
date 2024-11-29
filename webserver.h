#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include "json.hpp"
#include "crow_all.h"
#include "interfaces.h" // Assuming ICanvas is defined here

using namespace std;

class WebServer
{
private:
    struct HeaderMiddleware {
        struct context 
        {
        };

        void before_handle(crow::request& req, crow::response& res, context& ctx)
        {
        }

        void after_handle(crow::request& req, crow::response& res, context& ctx)
        {
            res.set_header("Content-Type", "application/json");
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        }
    };

    const vector<unique_ptr<ICanvas>> &_allCanvases; // Reference to all canvases
    crow::App<HeaderMiddleware> _crowApp;

public:
    WebServer(const vector<unique_ptr<ICanvas>> &allCanvases) : _allCanvases(allCanvases)
    {
    }

    ~WebServer()
    {
    }

    void Start()
    {
        // Define the `/api/sockets` endpoint
        CROW_ROUTE(_crowApp, "/api/sockets")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                auto socketsJson = nlohmann::json::array();
                for (size_t canvasId = 0; canvasId < _allCanvases.size(); ++canvasId)
                {
                    if (_allCanvases[canvasId]) // Ensure canvas exists
                    {
                        for (size_t featureId = 0; featureId < _allCanvases[canvasId]->Features().size(); ++featureId)
                        {
                            const auto& feature = _allCanvases[canvasId]->Features()[featureId];
                            nlohmann::json socketJson;
                            socketJson["hostName"] = feature->Socket().HostName();
                            socketJson["friendlyName"] = feature->Socket().FriendlyName();
                            socketJson["featureId"] = featureId;
                            socketJson["canvasId"] = _allCanvases[canvasId]->Id();
                            socketJson["isConnected"] = feature->Socket().IsConnected();
                            socketJson["bytesPerSecond"] = feature->Socket().BytesSentPerSecond();
                            socketJson["port"] = feature->Socket().Port();
                            socketsJson.push_back(socketJson);
                        }
                    }
                }
                return socketsJson.dump();
            });

        // Define the `/api/sockets/:id` endpoint
        CROW_ROUTE(_crowApp, "/api/sockets/<int>")
            .methods(crow::HTTPMethod::GET)([&](int id) -> crow::response
            {
                // Extract canvas and feature IDs from the socket ID
                int canvasId = id / 1000;
                int featureId = id % 1000;

                if (canvasId < 0 || canvasId >= _allCanvases.size() || !_allCanvases[canvasId])
                    return {crow::NOT_FOUND, R"({"error": "Socket not found - invalid canvas"})"};

                const auto& features = _allCanvases[canvasId]->Features();
                if (featureId < 0 || featureId >= features.size())
                    return {crow::NOT_FOUND, R"({"error": "Socket not found - invalid feature"})"};

                const auto& feature = features[featureId];
                nlohmann::json socketJson;
                socketJson["id"] = id;
                socketJson["canvasId"] = _allCanvases[canvasId]->Id();
                socketJson["featureId"] = featureId;
                socketJson["isConnected"] = feature->Socket().IsConnected();
                socketJson["port"] = feature->Socket().Port();
                
                // Add more detailed information for single socket view
                socketJson["bytesPerSecond"] = feature->Socket().BytesSentPerSecond();
                
                return socketJson.dump();
            });        
        // Define the `/api/canvases` endpoint
        CROW_ROUTE(_crowApp, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
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
                return canvasesJson.dump();
            });

        // Define the `/api/canvases/:id` endpoint
        CROW_ROUTE(_crowApp, "/api/canvases/<int>")
            .methods(crow::HTTPMethod::GET)([&](int id) -> crow::response
            {
                if (id < 0 || id >= _allCanvases.size() || !_allCanvases[id])
                    return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};

                nlohmann::json canvasJson;
                to_json(canvasJson, *_allCanvases[id]); // Use the utility function
                canvasJson["id"] = id; // Include ID in the details
                return canvasJson.dump();
            });

        // Start the server
        _crowApp.port(7777).multithreaded().run();
    }

    void Stop()
    {
        _crowApp.stop();
    }
};
