#include "kolosal/routes/engines_route.hpp"
#include "kolosal/utils.hpp"
#include "kolosal/server_api.hpp"
#include "kolosal/node_manager.h"
#include "kolosal/logger.hpp"
#include "kolosal/download_utils.hpp"
#include "kolosal/download_manager.hpp"
#include <json.hpp>
#include <iostream>
#include <thread>
#include <filesystem>

using json = nlohmann::json;

namespace kolosal
{
    // Initialize static regex patterns
    const std::regex EnginesRoute::enginesPattern_(R"(^/(v1/)?inference-engines/?$)");
    const std::regex EnginesRoute::engineStatusPattern_(R"(^/(v1/)?inference-engines/([^/]+)/status/?$)");
    const std::regex EnginesRoute::engineRescanPattern_(R"(^/(v1/)?inference-engines/rescan/?$)");

    bool EnginesRoute::match(const std::string &method, const std::string &path)
    {
        // Match inference engine-related endpoints:
        // GET/POST /inference-engines, /v1/inference-engines
        // GET /inference-engines/{id}/status, /v1/inference-engines/{id}/status
        // POST /inference-engines/rescan, /v1/inference-engines/rescan
        
        bool matches = false;
        
        if (std::regex_match(path, enginesPattern_))
        {
            matches = (method == "GET" || method == "POST");
        }
        else if (std::regex_match(path, engineStatusPattern_))
        {
            matches = (method == "GET");
        }
        else if (std::regex_match(path, engineRescanPattern_))
        {
            matches = (method == "POST");
        }
        
        // Store matched path and method for use in handle()
        if (matches)
        {
            matched_path_ = path;
            matched_method_ = method;
        }
        
        return matches;
    }

    void EnginesRoute::handle(SocketType sock, const std::string &body)
    {
        try
        {
            ServerLogger::logDebug("[Thread %u] Received %s request for path: %s", 
                                   std::this_thread::get_id(), matched_method_.c_str(), matched_path_.c_str());

            // Route to appropriate handler based on stored method and path
            if (std::regex_match(matched_path_, enginesPattern_))
            {
                if (matched_method_ == "GET")
                {
                    handleListEngines(sock, body);
                }
                else if (matched_method_ == "POST")
                {
                    handleDownloadEngine(sock, body);
                }
                else
                {
                    json jError = {{"error", {{"message", "Method not allowed"}, {"type", "method_not_allowed"}, {"param", nullptr}, {"code", nullptr}}}};
                    send_response(sock, 405, jError.dump());
                }
            }
            else if (std::regex_match(matched_path_, engineStatusPattern_))
            {
                if (matched_method_ == "GET")
                {
                    std::string engineName = extractEngineIdFromPath(matched_path_);
                    handleEngineStatus(sock, body, engineName);
                }
                else
                {
                    json jError = {{"error", {{"message", "Method not allowed"}, {"type", "method_not_allowed"}, {"param", nullptr}, {"code", nullptr}}}};
                    send_response(sock, 405, jError.dump());
                }
            }
            else if (std::regex_match(matched_path_, engineRescanPattern_))
            {
                if (matched_method_ == "POST")
                {
                    handleRescanEngines(sock, body);
                }
                else
                {
                    json jError = {{"error", {{"message", "Method not allowed"}, {"type", "method_not_allowed"}, {"param", nullptr}, {"code", nullptr}}}};
                    send_response(sock, 405, jError.dump());
                }
            }
            else
            {
                json jError = {{"error", {{"message", "Not found"}, {"type", "not_found"}, {"param", nullptr}, {"code", nullptr}}}};
                send_response(sock, 404, jError.dump());
            }
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error in EnginesRoute: %s", std::this_thread::get_id(), ex.what());
            json jError = {{"error", {{"message", std::string("Server error: ") + ex.what()}, {"type", "server_error"}, {"param", nullptr}, {"code", nullptr}}}};
            send_response(sock, 500, jError.dump());
        }
    }

    void EnginesRoute::handleListEngines(SocketType sock, const std::string &body)
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
            json jError = {{"error", {{"message", std::string("Server error: ") + ex.what()}, {"type", "server_error"}, {"param", nullptr}, {"code", nullptr}}}};
            send_response(sock, 500, jError.dump());
        }
    }

    void EnginesRoute::handleDownloadEngine(SocketType sock, const std::string &body)
    {
        try
        {
            ServerLogger::logInfo("[Thread %u] Received download engine request", std::this_thread::get_id());

            if (body.empty())
            {
                json jError = {{"error", {{"message", "Request body is required"}, {"type", "invalid_request_error"}, {"param", "body"}, {"code", "missing_body"}}}};
                send_response(sock, 400, jError.dump());
                return;
            }

            auto j = json::parse(body);

            // Required fields
            if (!j.contains("engine_name") || !j.contains("download_url"))
            {
                json jError = {{"error", {{"message", "Missing required fields: engine_name and download_url"}, {"type", "invalid_request_error"}, {"param", nullptr}, {"code", "missing_required_fields"}}}};
                send_response(sock, 400, jError.dump());
                return;
            }

            std::string engineName = j["engine_name"];
            std::string downloadUrl = j["download_url"];
            
            // Optional fields
            std::string description = j.value("description", "");
            std::string version = j.value("version", "unknown");
            bool loadImmediately = j.value("load_immediately", false);

            // Validate engine name
            if (engineName.empty())
            {
                json jError = {{"error", {{"message", "Engine name cannot be empty"}, {"type", "invalid_request_error"}, {"param", "engine_name"}, {"code", "invalid_engine_name"}}}};
                send_response(sock, 400, jError.dump());
                return;
            }

            // Validate download URL
            if (!is_valid_url(downloadUrl))
            {
                json jError = {{"error", {{"message", "Invalid download URL format"}, {"type", "invalid_request_error"}, {"param", "download_url"}, {"code", "invalid_url"}}}};
                send_response(sock, 400, jError.dump());
                return;
            }

            // Get the filename from URL and ensure it has proper extension
            std::string filename = std::filesystem::path(downloadUrl).filename().string();
            
            // Add platform-specific extension if missing
            #ifdef _WIN32
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".dll")
            {
                filename += ".dll";
            }
            #else
            if (filename.size() < 3 || filename.substr(filename.size() - 3) != ".so")
            {
                filename += ".so";
            }
            #endif

            // Set download path to plugins directory
            std::string pluginsDir = "./plugins";
            std::string downloadPath = pluginsDir + "/" + filename;
            
            // Ensure plugins directory exists
            std::filesystem::create_directories(pluginsDir);

            // Check if engine already exists
            auto &nodeManager = ServerAPI::instance().getNodeManager();
            auto availableEngines = nodeManager.getAvailableInferenceEngines();
            
            for (const auto &engine : availableEngines)
            {
                if (engine.name == engineName)
                {
                    json jError = {{"error", {{"message", "Engine with this name already exists"}, {"type", "conflict_error"}, {"param", "engine_name"}, {"code", "engine_already_exists"}}}};
                    send_response(sock, 409, jError.dump());
                    return;
                }
            }

            // Use engineName as the download ID for the download manager
            std::string downloadId = "engine_" + engineName;

            // Check if file already exists and is complete
            if (std::filesystem::exists(downloadPath))
            {
                auto &download_manager = DownloadManager::getInstance();
                auto previous_download = download_manager.getDownloadProgress(downloadId);
                
                bool fileIsComplete = false;
                if (previous_download && previous_download->total_bytes > 0)
                {
                    // Get actual file size
                    std::uintmax_t fileSize = std::filesystem::file_size(downloadPath);
                    
                    if (fileSize >= previous_download->total_bytes)
                    {
                        fileIsComplete = true;
                        ServerLogger::logInfo("[Thread %u] Engine file '%s' already exists and appears complete", 
                                             std::this_thread::get_id(), downloadPath.c_str());
                    }
                    else
                    {
                        ServerLogger::logInfo("[Thread %u] Engine file '%s' exists but appears incomplete (%zu/%zu bytes), will resume download", 
                                             std::this_thread::get_id(), downloadPath.c_str(), fileSize, previous_download->total_bytes);
                    }
                }
                else
                {
                    // No previous download record - assume file is complete if it exists and has reasonable size
                    std::uintmax_t fileSize = std::filesystem::file_size(downloadPath);
                    if (fileSize > 1024 * 1024) // Assume files larger than 1MB are complete engine libraries
                    {
                        fileIsComplete = true;
                        ServerLogger::logInfo("[Thread %u] Engine file '%s' already exists (size: %zu bytes)", 
                                             std::this_thread::get_id(), downloadPath.c_str(), fileSize);
                    }
                    else
                    {
                        ServerLogger::logInfo("[Thread %u] Engine file '%s' exists but is too small (%zu bytes), will re-download", 
                                             std::this_thread::get_id(), downloadPath.c_str(), fileSize);
                    }
                }
                
                if (fileIsComplete)
                {
                    // File is complete, try to rescan and load the engine if requested
                    if (loadImmediately)
                    {
                        // Rescan for engines to pick up the new library
                        bool rescanSuccess = nodeManager.rescanInferenceEngines();
                        if (rescanSuccess)
                        {
                            // Try to load the engine
                            bool loadSuccess = nodeManager.loadInferenceEngine(engineName);
                            if (loadSuccess)
                            {
                                json response = {
                                    {"engine_name", engineName},
                                    {"status", "loaded"},
                                    {"message", "Engine library was already downloaded and loaded successfully"},
                                    {"download_url", downloadUrl},
                                    {"local_path", downloadPath},
                                    {"load_immediately", loadImmediately}
                                };
                                send_response(sock, 200, response.dump());
                                ServerLogger::logInfo("[Thread %u] Engine '%s' library was already downloaded and loaded successfully", 
                                                     std::this_thread::get_id(), engineName.c_str());
                            }
                            else
                            {
                                json response = {
                                    {"engine_name", engineName},
                                    {"status", "downloaded"},
                                    {"message", "Engine library was already downloaded and rescan completed, but loading failed. Check server logs for details."},
                                    {"download_url", downloadUrl},
                                    {"local_path", downloadPath},
                                    {"load_immediately", loadImmediately}
                                };
                                send_response(sock, 200, response.dump());
                                ServerLogger::logWarning("[Thread %u] Engine '%s' library was downloaded but failed to load", 
                                                        std::this_thread::get_id(), engineName.c_str());
                            }
                        }
                        else
                        {
                            json response = {
                                {"engine_name", engineName},
                                {"status", "downloaded"},
                                {"message", "Engine library was already downloaded but rescan failed. Restart the server to load it."},
                                {"download_url", downloadUrl},
                                {"local_path", downloadPath},
                                {"load_immediately", loadImmediately}
                            };
                            send_response(sock, 200, response.dump());
                            ServerLogger::logWarning("[Thread %u] Engine '%s' library was downloaded but rescan failed", 
                                                    std::this_thread::get_id(), engineName.c_str());
                        }
                    }
                    else
                    {
                        json response = {
                            {"engine_name", engineName},
                            {"status", "downloaded"},
                            {"message", "Engine library was already downloaded"},
                            {"download_url", downloadUrl},
                            {"local_path", downloadPath},
                            {"load_immediately", loadImmediately}
                        };
                        send_response(sock, 200, response.dump());
                        ServerLogger::logInfo("[Thread %u] Engine '%s' library was already downloaded", 
                                             std::this_thread::get_id(), engineName.c_str());
                    }
                    return;
                }
            }

            // Start download if file doesn't exist or is incomplete
            auto &download_manager = DownloadManager::getInstance();

            // Check if there's a previous download that was cancelled or failed
            auto previous_download = download_manager.getDownloadProgress(downloadId);
            if (previous_download && (previous_download->cancelled || previous_download->status == "failed"))
            {
                ServerLogger::logInfo("[Thread %u] Previous download for engine '%s' was cancelled or failed, restarting", 
                                     std::this_thread::get_id(), engineName.c_str());
                download_manager.cancelDownload(downloadId);
            }

            bool download_started = download_manager.startDownload(downloadId, downloadUrl, downloadPath);

            if (!download_started)
            {
                // Check if download is already in progress
                if (download_manager.isDownloadInProgress(downloadId))
                {
                    json jResponse = {
                        {"engine_name", engineName},
                        {"status", "downloading"},
                        {"message", "Download is already in progress"},
                        {"download_url", downloadUrl},
                        {"local_path", downloadPath}
                    };
                    send_response(sock, 200, jResponse.dump());
                    ServerLogger::logInfo("[Thread %u] Download already in progress for engine %s", 
                                         std::this_thread::get_id(), engineName.c_str());
                }
                else
                {
                    json jError = {{"error", {{"message", "Failed to start download"}, {"type", "download_error"}, {"param", nullptr}, {"code", "download_start_failed"}}}};
                    send_response(sock, 500, jError.dump());
                    ServerLogger::logError("[Thread %u] Failed to start download for engine %s", 
                                          std::this_thread::get_id(), engineName.c_str());
                }
            }
            else
            {
                // Return 202 Accepted for async download
                std::string message;
                if (loadImmediately)
                {
                    message = "Engine library download started in background. Check status to monitor progress and loading.";
                }
                else
                {
                    message = "Engine library download started in background. Use rescan endpoint or restart server after download completes to load the engine.";
                }
                
                json jResponse = {
                    {"engine_name", engineName},
                    {"status", "downloading"},
                    {"message", message},
                    {"download_url", downloadUrl},
                    {"local_path", downloadPath},
                    {"load_immediately", loadImmediately}
                };

                send_response(sock, 202, jResponse.dump());
                ServerLogger::logInfo("[Thread %u] Started async download for engine %s from URL: %s", 
                                     std::this_thread::get_id(), engineName.c_str(), downloadUrl.c_str());
            }
        }
        catch (const json::exception &ex)
        {
            ServerLogger::logError("[Thread %u] JSON parsing error: %s", std::this_thread::get_id(), ex.what());
            json jError = {{"error", {{"message", std::string("Invalid JSON: ") + ex.what()}, {"type", "invalid_request_error"}, {"param", nullptr}, {"code", nullptr}}}};
            send_response(sock, 400, jError.dump());
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error handling download engine request: %s", std::this_thread::get_id(), ex.what());
            json jError = {{"error", {{"message", std::string("Server error: ") + ex.what()}, {"type", "server_error"}, {"param", nullptr}, {"code", nullptr}}}};
            send_response(sock, 500, jError.dump());
        }
    }

    void EnginesRoute::handleEngineStatus(SocketType sock, const std::string &body, const std::string &engineName)
    {
        try
        {
            ServerLogger::logInfo("[Thread %u] Received engine status request for engine: %s", std::this_thread::get_id(), engineName.c_str());

            // Check if there's an active download for this engine (using the download ID format)
            std::string downloadId = "engine_" + engineName;
            auto &download_manager = DownloadManager::getInstance();
            auto downloadProgress = download_manager.getDownloadProgress(downloadId);

            if (downloadProgress)
            {
                // Engine library is being downloaded
                json response = {
                    {"engine_name", engineName},
                    {"status", downloadProgress->status},
                    {"download_progress", {
                        {"percentage", downloadProgress->percentage},
                        {"downloaded_bytes", downloadProgress->downloaded_bytes},
                        {"total_bytes", downloadProgress->total_bytes},
                        {"url", downloadProgress->url},
                        {"local_path", downloadProgress->local_path}
                    }},
                    {"message", "Engine library download in progress"}
                };

                if (!downloadProgress->error_message.empty())
                {
                    response["error_message"] = downloadProgress->error_message;
                }

                send_response(sock, 200, response.dump());
                ServerLogger::logInfo("[Thread %u] Engine '%s' download status: %s (%.1f%%)", 
                                     std::this_thread::get_id(), engineName.c_str(), 
                                     downloadProgress->status.c_str(), downloadProgress->percentage);
                return;
            }

            // Check if engine is available/loaded
            auto &nodeManager = ServerAPI::instance().getNodeManager();
            auto availableEngines = nodeManager.getAvailableInferenceEngines();

            for (const auto &engine : availableEngines)
            {
                if (engine.name == engineName)
                {
                    json response = {
                        {"engine_name", engineName},
                        {"status", engine.is_loaded ? "loaded" : "available"},
                        {"available", true},
                        {"engine_info", {
                            {"name", engine.name},
                            {"version", engine.version},
                            {"description", engine.description},
                            {"library_path", engine.library_path}
                        }},
                        {"message", engine.is_loaded ? "Engine is loaded and ready" : "Engine is available but not loaded"}
                    };

                    send_response(sock, 200, response.dump());
                    ServerLogger::logInfo("[Thread %u] Successfully retrieved status for engine '%s'", 
                                         std::this_thread::get_id(), engineName.c_str());
                    return;
                }
            }

            // Engine not found
            json errorResponse = {
                {"error", {
                    {"message", "Engine not found"},
                    {"type", "not_found_error"},
                    {"param", "engine_name"},
                    {"code", "engine_not_found"}
                }}
            };

            send_response(sock, 404, errorResponse.dump());
            ServerLogger::logWarning("[Thread %u] Engine '%s' not found", std::this_thread::get_id(), engineName.c_str());
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error handling engine status request: %s", std::this_thread::get_id(), ex.what());
            json jError = {{"error", {{"message", std::string("Server error: ") + ex.what()}, {"type", "server_error"}, {"param", nullptr}, {"code", nullptr}}}};
            send_response(sock, 500, jError.dump());
        }
    }

    std::string EnginesRoute::extractEngineIdFromPath(const std::string &path)
    {
        std::smatch matches;
        
        // Try engine status pattern
        if (std::regex_match(path, matches, engineStatusPattern_))
        {
            return matches[2].str(); // Group 2 contains the engine ID
        }
        
        return "";
    }

    void EnginesRoute::handleRescanEngines(SocketType sock, const std::string &body)
    {
        try
        {
            ServerLogger::logInfo("[Thread %u] Received rescan engines request", std::this_thread::get_id());

            // Get the NodeManager and trigger a rescan
            auto &nodeManager = ServerAPI::instance().getNodeManager();
            bool success = nodeManager.rescanInferenceEngines();

            if (success)
            {
                // Get the updated list of engines
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
                    {"success", true},
                    {"message", "Rescan completed successfully"},
                    {"inference_engines", enginesList},
                    {"total_count", enginesList.size()}
                };

                send_response(sock, 200, response.dump());
                ServerLogger::logInfo("[Thread %u] Successfully rescanned engines. Found %zu engines", 
                                     std::this_thread::get_id(), enginesList.size());
            }
            else
            {
                json errorResponse = {
                    {"error", {
                        {"message", "Failed to rescan for inference engines"},
                        {"type", "rescan_error"},
                        {"param", nullptr},
                        {"code", "rescan_failed"}
                    }}
                };

                send_response(sock, 500, errorResponse.dump());
                ServerLogger::logError("[Thread %u] Failed to rescan for inference engines", std::this_thread::get_id());
            }
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("[Thread %u] Error handling rescan engines request: %s", std::this_thread::get_id(), ex.what());
            json jError = {{"error", {{"message", std::string("Server error: ") + ex.what()}, {"type", "server_error"}, {"param", nullptr}, {"code", nullptr}}}};
            send_response(sock, 500, jError.dump());
        }
    }

} // namespace kolosal
