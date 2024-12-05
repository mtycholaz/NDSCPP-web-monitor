#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <ranges>
#include "json.hpp"
#include "crow_all.h"
#include "interfaces.h" // Assuming ICanvas is defined here
#include "serialization.h"

using namespace std;

class WebServer
{
private:
    struct HeaderMiddleware
    {
        struct context
        {
        };

        void before_handle(crow::request &req, crow::response &res, context &ctx)
        {
        }

        void after_handle(crow::request &req, crow::response &res, context &ctx)
        {
            res.set_header("Content-Type", "application/json");
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Methods", "GET, OPTIONS");
        }
    };

    vector<unique_ptr<ICanvas>> &_allCanvases; // Reference to all canvases
    crow::App<HeaderMiddleware> _crowApp;

public:
    WebServer(vector<unique_ptr<ICanvas>> &allCanvases) : _allCanvases(allCanvases)
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
                
                for (size_t canvasId = 0; canvasId < _allCanvases.size(); ++canvasId) {
                    if (!_allCanvases[canvasId]) {
                        continue;
                    }

                    for (size_t featureId = 0; featureId < _allCanvases[canvasId]->Features().size(); ++featureId) {
                        nlohmann::json socketJson = _allCanvases[canvasId]->Features()[featureId]->Socket();
                        socketJson["featureId"] = featureId;
                        socketJson["canvasId"] = _allCanvases[canvasId]->Id();
                        socketsJson.push_back(socketJson);
                    }
                }
                
                return socketsJson.dump(); 
            });

        // Define the `/api/sockets/:id` endpoint
        CROW_ROUTE(_crowApp, "/api/sockets/<int>")
            .methods(crow::HTTPMethod::GET)([&](int socketId) -> crow::response
            {
                // Search through all canvases and features to find the matching socket
                for (size_t canvasId = 0; canvasId < _allCanvases.size(); ++canvasId) {
                    if (!_allCanvases[canvasId]) {
                        continue;
                    }

                    for (size_t featureId = 0; featureId < _allCanvases[canvasId]->Features().size(); ++featureId) {
                        const auto& feature = _allCanvases[canvasId]->Features()[featureId];
                        
                        // Check if this socket matches the requested ID
                        if (feature->Socket().Id() == socketId) {
                            nlohmann::json socketJson = feature->Socket();
                            
                            // Add the contextual fields
                            socketJson["featureId"] = featureId;
                            socketJson["canvasId"] = _allCanvases[canvasId]->Id();

                            return socketJson.dump();
                        }
                    }
                }
                
                // If we didn't find the socket, return a 404
                return {crow::NOT_FOUND, R"({"error": "Socket not found"})"}; 
            });

        CROW_ROUTE(_crowApp, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                auto canvasesJson = nlohmann::json::array();
                for (size_t i = 0; i < _allCanvases.size(); ++i)
                {
                    if (_allCanvases[i]) // Ensure the canvas pointer is valid
                    {
                        nlohmann::json canvasJson = *_allCanvases[i]; // Use the utility function
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

                nlohmann::json canvasJson = *_allCanvases[id]; // Use the utility function
                canvasJson["id"] = id; // Include ID in the details
                return canvasJson.dump(); 
            });
            
            // Create new canvas
            CROW_ROUTE(_crowApp, "/api/canvases")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req) -> crow::response 
                {
                    auto reqJson = nlohmann::json::parse(req.body);
                    
                    size_t canvasId = _allCanvases.size();
                    _allCanvases.push_back(reqJson.get<unique_ptr<ICanvas>>());
                    
                    nlohmann::json response;
                    response["id"] = canvasId;
                    return response.dump();
                });

            // Delete canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int id) -> crow::response 
                {
                    if (id < 0 || id >= _allCanvases.size() || !_allCanvases[id])
                        return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};
                    
                    _allCanvases[id].reset();
                    return crow::response(crow::OK);
                });

            // Create feature and add to canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req, int canvasId) -> crow::response 
                {
                    if (canvasId < 0 || canvasId >= _allCanvases.size() || !_allCanvases[canvasId])
                        return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};
                    
                    auto reqJson = nlohmann::json::parse(req.body);
                    
                    size_t featureId = _allCanvases[canvasId]->Features().size();
                    _allCanvases[canvasId]->AddFeature(reqJson.get<unique_ptr<ILEDFeature>>());
                    
                    nlohmann::json response;
                    response["id"] = featureId;
                    return response.dump();
                });

            // Delete feature from canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int canvasId, int featureId) -> crow::response {
                    if (canvasId < 0 || canvasId >= _allCanvases.size() || !_allCanvases[canvasId])
                        return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};
                        
                    auto& features = _allCanvases[canvasId]->Features();
                    if (featureId < 0 || featureId >= features.size())
                        return {crow::NOT_FOUND, R"({"error": "Feature not found"})"};
                        
                    _allCanvases[canvasId]->RemoveFeature(features[featureId]);
                    return crow::response(crow::OK);
                });
                
        // Start the server
        _crowApp.port(7777).multithreaded().run();
    }

    void Stop()
    {
        _crowApp.stop();
    }
};
