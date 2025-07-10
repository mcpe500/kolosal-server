#include "kolosal/routes/list_models_route.hpp"
#include "kolosal/utils.hpp"
#include "kolosal/server_api.hpp"
#include "kolosal/node_manager.h"
#include "kolosal/logger.hpp"
#include <json.hpp>
#include <iostream>
#include <thread>

using json = nlohmann::json;

namespace kolosal
{

    bool ListModelsRoute::match(const std::string &method, const std::string &path)
    {
        return (method == "GET" && (path == "/models" || path == "/v1/models"));
    }

    void ListModelsRoute::handle(SocketType sock, const std::string &body)
    {
        try
        {
            ServerLogger::logDebug("[Thread %u] Received list models request", std::this_thread::get_id());

            // Get the NodeManager and list engines
            auto &nodeManager = ServerAPI::instance().getNodeManager();
            auto engineIds = nodeManager.listEngineIds();

            json modelsList = json::array();
            for (const auto &engineId : engineIds)
            {
                // Check engine status without loading it (to avoid triggering lazy model loading)
                auto [exists, isLoaded] = nodeManager.getEngineStatus(engineId);

                json modelInfo = {
                    {"model_id", engineId},
                    {"status", isLoaded ? "loaded" : "unloaded"},
                    {"last_accessed", "recently"} // Could be enhanced with actual timestamps
                };

                modelsList.push_back(modelInfo);
            }

            json response = {
                {"models", modelsList},
                {"total_count", modelsList.size()}};

            send_response(sock, 200, response.dump());
            ServerLogger::logDebug("[Thread %u] Successfully listed %zu models", std::this_thread::get_id(), modelsList.size());
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error handling list models request: %s", std::this_thread::get_id(), ex.what());

            json jError = {{"error", {{"message", std::string("Server error: ") + ex.what()}, {"type", "server_error"}, {"param", nullptr}, {"code", nullptr}}}};

            send_response(sock, 500, jError.dump());
        }
    }

} // namespace kolosal
