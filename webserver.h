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
            res.add_header("Access-Control-Allow-Methods", "GET, OPTIONS, POST, DELETE");
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
                try
                {
                    return crow::response(nlohmann::json{{"controller", _controller}}.dump());
                }
                catch(const std::exception& e)
                {
                    logger->error("Error in /api/controller: {}", e.what());
                    return {400, string("Error: ") + e.what()};
                }
            });

        // Enumerate just the sockets

        CROW_ROUTE(_crowApp, "/api/sockets")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                try
                {
                    return crow::response(nlohmann::json{{"sockets", _controller.GetSockets()}}.dump());
                }
                catch(const std::exception& e)
                {
                    logger->error("Error in /api/sockets: {}", e.what());
                    return {400, string("Error: ") + e.what()};
                }
            });


        // Detail a single socket

        CROW_ROUTE(_crowApp, "/api/sockets/<int>") 
            .methods(crow::HTTPMethod::GET)([&](int socketId) -> crow::response
            {
                try
                {
                    return crow::response(nlohmann::json{{"socket", *_controller.GetSocketById(socketId)}}.dump());
                }
                catch(const std::exception& e)
                {
                    logger->error("Error in /api/sockets/{}: {}", socketId, e.what());
                    return {400, string("Error: ") + e.what()};
                }
            });

        // Enumerate all the canvases

        CROW_ROUTE(_crowApp, "/api/canvases")
            .methods(crow::HTTPMethod::GET)([&]() -> crow::response
            {
                try
                {
                    return nlohmann::json(_controller.Canvases()).dump();
                }
                catch(const std::exception& e)
                {
                    logger->error("Error in /api/canvases: {}", e.what());
                    return {400, string("Error: ") + e.what()};
                }
            });

        // Detail a single canvas

        CROW_ROUTE(_crowApp, "/api/canvases/<int>")
            .methods(crow::HTTPMethod::GET)([&](int id) -> crow::response
            {
                try
                {
                    auto allCanvases = _controller.Canvases();
                    if (id < 0 || id >= allCanvases.size())
                        return {crow::NOT_FOUND, R"({"error": "Canvas not found"})"};

                    return nlohmann::json(*_controller.GetCanvasById(id)).dump(); 
                }
                catch(const std::exception& e)
                {
                    logger->error("Error in /api/canvases/{}: {}", id, e.what());
                    return {400, string("Error: ") + e.what()};
                }
                
            });
            
            // Create new canvas
            CROW_ROUTE(_crowApp, "/api/canvases")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req) -> crow::response
                {
                    try 
                    {
                        // Parse and deserialize JSON payload
                        auto jsonPayload = nlohmann::json::parse(req.body);
                        uint32_t newID = _controller.AddCanvas(jsonPayload.get<shared_ptr<ICanvas>>());
                        if (newID == -1)
                            return {400, "Error, likely canvas with that ID already exists."};

                        return crow::response(201, nlohmann::json{{"id", newID}}.dump());                    
                    } 
                    catch (const exception& e) 
                    {
                        logger->error("Error in /api/canvases POST: {}", e.what());
                        return {400, string("Error: ") + e.what()};
                    }
                });

            // Create feature and add to canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features")
                .methods(crow::HTTPMethod::POST)([&](const crow::request& req, int canvasId) -> crow::response
                {
                    try 
                    {
                        auto reqJson = nlohmann::json::parse(req.body);
                        auto feature = reqJson.get<shared_ptr<ILEDFeature>>();
                        auto newId = _controller.GetCanvasById(canvasId)->AddFeature(feature);
                        return nlohmann::json{{"id", newId}}.dump();

                    } 
                    catch (const exception& e) 
                    {
                        logger->error("Error in /api/canvases/{}/features POST: {}", canvasId, e.what());
                        return {400, string("Error: ") + e.what()};
                    }
                });


            // Delete feature from canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>/features/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int canvasId, int featureId)
                {
                    try
                    {
                        auto canvas = _controller.GetCanvasById(canvasId);
                        canvas->RemoveFeatureById(featureId);
                        return crow::response(crow::OK);
                    }
                    catch(const std::exception& e)
                    {
                        logger->error("Error in /api/canvases/{}/features/{} DELETE: {}", canvasId, featureId, e.what());
                        return crow::response(crow::BAD_REQUEST, string("Error: ") + e.what());
                    }
                });
                

            // Delete canvas
            CROW_ROUTE(_crowApp, "/api/canvases/<int>")
                .methods(crow::HTTPMethod::DELETE)([&](int id)
                {
                    try
                    {
                        _controller.DeleteCanvasById(id);
                        return crow::response(crow::OK);
                    }
                    catch(const std::exception& e)
                    {
                        logger->error("Error in /api/canvases/{} DELETE: {}", id, e.what());
                        return crow::response(crow::BAD_REQUEST, string("Error: ") + e.what());
                    }
                });

        // Start the server
        _crowApp.port(_controller.GetPort()).multithreaded().run();
    }

    void Stop()
    {
        _crowApp.stop();
    }
};
