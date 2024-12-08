#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include <ranges>
#include "json.hpp"
#include "crow_all.h"
#include "controller.h"

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

    IController & _controller; // Reference to all canvases
    crow::App<HeaderMiddleware> _crowApp;

public:
    WebServer(IController & controller) : _controller(controller)
    {
    }

    ~WebServer()
    {
    }

    void Start()
    {
        // The main controller, the most info you can get in a single call

        CROW_ROUTE(_crowApp, "/api/controller")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                nlohmann::json response;
                response["controller"] = _controller;
                return response.dump();

            });

        // Enumerate just the sockets

        CROW_ROUTE(_crowApp, "/api/sockets")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                auto socketsJson = nlohmann::json::array();
                for (const auto &socket : _controller.GetSockets())
                    socketsJson.push_back(socket); // Assumes `to_json` is defined for `socket`

                return socketsJson.dump();
            });


        // Detail a single socket

        CROW_ROUTE(_crowApp, "/api/sockets/<int>") 
            .methods(crow::HTTPMethod::GET)([&](int socketId) -> crow::response
            {
                return nlohmann::json(_controller.GetSocketById(socketId)).dump();
            });

        // Enumerate all the canvases

        CROW_ROUTE(_crowApp, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                return nlohmann::json(_controller.Canvases()).dump();
            });

        // Detail a single canvas

        CROW_ROUTE(_crowApp, "/api/canvases/<int>")
            .methods(crow::HTTPMethod::GET)([&](int id) -> crow::response
            {
                auto allCanvases = _controller.Canvases();
                if (id < 0 || id >= allCanvases.size())
                    return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};

                return nlohmann::json(allCanvases[id]).dump(); 
            });
            
            // Create new canvas
            CROW_ROUTE(_crowApp, "/api/canvases")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req) -> crow::response
                {
                    try 
                    {
                        // Parse and deserialize JSON payload
                        auto jsonPayload = nlohmann::json::parse(req.body);

                        // Add the canvas to the controller
                        if (!_controller.AddCanvas(jsonPayload.get<unique_ptr<ICanvas>>()))
                            return {400, "Error, likely canvas with that ID already exists."};
                        return {201, "Canvas added successfully."};
                    } 
                    catch (const exception& e) 
                    {
                        return {400, string("Error: ") + e.what()};
                    }
                });



            // Delete canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int id)
                {
                    _controller.DeleteCanvasById(id);
                    return crow::response(crow::OK);
                });

            // Create feature and add to canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req, int canvasId) -> crow::response
                {
                    try 
                    {
                        auto reqJson = nlohmann::json::parse(req.body);
                        auto feature = reqJson.get<unique_ptr<ILEDFeature>>();
                        auto newId = _controller.Canvases()[canvasId].get().AddFeature(std::move(feature));
                        return nlohmann::json{{"id", newId}}.dump();
                    } 
                    catch (const exception& e) 
                    {
                        return {400, string("Error: ") + e.what()};
                    }
                });


            // Delete feature from canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int canvasId, int featureId)
                {
                    _controller.GetCanvasById(canvasId).RemoveFeatureById(featureId);
                    auto canvas = _controller.Canvases()[canvasId];
                    canvas.get().RemoveFeatureById(featureId);
                    return crow::response(crow::OK);
                });
                
        // Start the server
        _crowApp.port(_controller.GetPort()).multithreaded().run();
    }

    void Stop()
    {
        _crowApp.stop();
    }
};
