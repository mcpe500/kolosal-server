#include "kolosal/routes/engines_route.hpp"
#include "kolosal/utils.hpp"
#include "kolosal/server_api.hpp"
#include "kolosal/node_manager.h"
#include "kolosal/server_config.hpp"
#include "kolosal/logger.hpp"
#include <json.hpp>
#include <iostream>
#include <thread>
#include <filesystem>

using json = nlohmann::json;

namespace kolosal
{

    bool EnginesRoute::match(const std::string &method, const std::string &path)
    {
        bool matches = ((method == "GET" || method == "POST") && (path == "/engines" || path == "/v1/engines"));
        
        // Store matched method for use in handle()
        if (matches)
        {
            matched_method_ = method;
        }
        
        return matches;
    }

    void EnginesRoute::handle(SocketType sock, const std::string &body)
    {
        // Route to appropriate handler based on method
        if (matched_method_ == "GET")
        {
            handleGetEngines(sock);
        }
        else if (matched_method_ == "POST")
        {
            handleAddEngine(sock, body);
        }
        else
        {
            json jError = {
                {"error", {
                    {"message", "Method not allowed. Use GET to list engines or POST to add engines."}, 
                    {"type", "method_not_allowed"}, 
                    {"param", nullptr}, 
                    {"code", nullptr}
                }}
            };
            send_response(sock, 405, jError.dump());
        }
    }

    void EnginesRoute::handleGetEngines(SocketType sock)
    {
        try
        {
            ServerLogger::logDebug("[Thread %u] Received list inference engines request", std::this_thread::get_id());

            // Get the NodeManager and list available inference engines
            auto &nodeManager = ServerAPI::instance().getNodeManager();
            auto availableEngines = nodeManager.getAvailableInferenceEngines();

            json enginesList = json::array();
            for (const auto &engine : availableEngines)
            {
                json engineInfo = {
                    {"name", engine.name},
                    {"version", engine.version},
                    {"description", engine.description},
                    {"library_path", engine.library_path},
                    {"is_loaded", engine.is_loaded}
                };

                enginesList.push_back(engineInfo);
            }

            json response = {
                {"inference_engines", enginesList},
                {"total_count", enginesList.size()}
            };

            send_response(sock, 200, response.dump());
            ServerLogger::logDebug("[Thread %u] Successfully listed %zu inference engines", std::this_thread::get_id(), enginesList.size());
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error handling list inference engines request: %s", std::this_thread::get_id(), ex.what());

            json jError = {
                {"error", {
                    {"message", std::string("Server error: ") + ex.what()}, 
                    {"type", "server_error"}, 
                    {"param", nullptr}, 
                    {"code", nullptr}
                }}
            };

            send_response(sock, 500, jError.dump());
        }
    }

    void EnginesRoute::handleAddEngine(SocketType sock, const std::string &body)
    {
        try
        {
            ServerLogger::logDebug("[Thread %u] Received add inference engine request", std::this_thread::get_id());

            // Parse JSON request body
            json requestData;
            try
            {
                requestData = json::parse(body);
            }
            catch (const json::parse_error &e)
            {
                json jError = {
                    {"error", {
                        {"message", "Invalid JSON in request body"}, 
                        {"type", "invalid_request_error"}, 
                        {"param", "body"}, 
                        {"code", nullptr}
                    }}
                };
                send_response(sock, 400, jError.dump());
                return;
            }

            // Validate required fields
            if (!requestData.contains("name") || !requestData.contains("library_path"))
            {
                json jError = {
                    {"error", {
                        {"message", "Missing required fields: 'name' and 'library_path' are required"}, 
                        {"type", "invalid_request_error"}, 
                        {"param", "body"}, 
                        {"code", nullptr}
                    }}
                };
                send_response(sock, 400, jError.dump());
                return;
            }

            // Extract engine configuration
            std::string engineName = requestData["name"];
            std::string libraryPath = requestData["library_path"];
            std::string description = requestData.value("description", "");
            bool loadOnStartup = requestData.value("load_on_startup", true);

            // Validate engine name uniqueness
            auto& config = ServerConfig::getInstance();
            for (const auto& existingEngine : config.inferenceEngines)
            {
                if (existingEngine.name == engineName)
                {
                    json jError = {
                        {"error", {
                            {"message", "Engine with name '" + engineName + "' already exists"}, 
                            {"type", "invalid_request_error"}, 
                            {"param", "name"}, 
                            {"code", nullptr}
                        }}
                    };
                    send_response(sock, 409, jError.dump());
                    return;
                }
            }

            // Validate library path exists
            if (!std::filesystem::exists(libraryPath))
            {
                json jError = {
                    {"error", {
                        {"message", "Library file not found: " + libraryPath}, 
                        {"type", "invalid_request_error"}, 
                        {"param", "library_path"}, 
                        {"code", nullptr}
                    }}
                };
                send_response(sock, 400, jError.dump());
                return;
            }

            // Create new engine configuration
            InferenceEngineConfig newEngine(engineName, libraryPath, description);
            newEngine.load_on_startup = loadOnStartup;

            // Add to server config
            config.inferenceEngines.push_back(newEngine);

            // Save updated config to file
            std::string configFile = "config.yaml"; // Default config file
            if (!config.saveToFile(configFile))
            {
                // Remove the engine from memory if save failed
                config.inferenceEngines.pop_back();
                
                json jError = {
                    {"error", {
                        {"message", "Failed to save configuration to file"}, 
                        {"type", "server_error"}, 
                        {"param", nullptr}, 
                        {"code", nullptr}
                    }}
                };
                send_response(sock, 500, jError.dump());
                return;
            }

            // Reconfigure inference loader with updated engines
            auto& nodeManager = ServerAPI::instance().getNodeManager();
            
            if (!nodeManager.reconfigureEngines(config.inferenceEngines))
            {
                ServerLogger::logWarning("Failed to reconfigure inference engines after adding new engine");
            }

            // Prepare response
            json engineInfo = {
                {"name", newEngine.name},
                {"library_path", newEngine.library_path},
                {"description", newEngine.description},
                {"load_on_startup", newEngine.load_on_startup},
                {"is_loaded", false} // Newly added engines are not loaded by default
            };

            json response = {
                {"message", "Inference engine added successfully"},
                {"engine", engineInfo}
            };

            send_response(sock, 201, response.dump());
            ServerLogger::logInfo("[Thread %u] Successfully added inference engine: %s", std::this_thread::get_id(), engineName.c_str());
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error handling add inference engine request: %s", std::this_thread::get_id(), ex.what());

            json jError = {
                {"error", {
                    {"message", std::string("Server error: ") + ex.what()}, 
                    {"type", "server_error"}, 
                    {"param", nullptr}, 
                    {"code", nullptr}
                }}
            };

            send_response(sock, 500, jError.dump());
        }
    }

} // namespace kolosal
