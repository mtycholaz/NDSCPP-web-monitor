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
            res.manual_length_header = true;
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
