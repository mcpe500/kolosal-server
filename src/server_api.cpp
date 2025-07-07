#include "kolosal/server_api.hpp"
#include "kolosal/server.hpp"
#include "kolosal/routes/chat_completion_route.hpp"
#include "kolosal/routes/completion_route.hpp"
#include "kolosal/routes/embedding_route.hpp"
#include "kolosal/routes/models_route.hpp"
#include "kolosal/routes/inference_completion_route.hpp"
#include "kolosal/routes/inference_chat_completion_route.hpp"
#include "kolosal/routes/add_engine_route.hpp"
#include "kolosal/routes/list_engines_route.hpp"
#include "kolosal/routes/remove_engine_route.hpp"
#include "kolosal/routes/engine_status_route.hpp"
#include "kolosal/routes/health_status_route.hpp"
#include "kolosal/routes/auth_config_route.hpp"

#include "kolosal/routes/download_progress_route.hpp"
#include "kolosal/routes/downloads_status_route.hpp"
#include "kolosal/routes/cancel_download_route.hpp"
#include "kolosal/routes/pause_download_route.hpp"
#include "kolosal/routes/resume_download_route.hpp"
#include "kolosal/routes/cancel_all_downloads_route.hpp"
#include "kolosal/routes/parse_pdf_route.hpp"
#include "kolosal/routes/parse_docx_route.hpp"
#include "kolosal/routes/add_documents_route.hpp"
#include "kolosal/retrieval/remove_documents_route.hpp"
#include "kolosal/routes/retrieve_route.hpp"
#include "kolosal/routes/internet_search_route.hpp"
#include "kolosal/download_manager.hpp"
#include "kolosal/node_manager.h"
#include "kolosal/logger.hpp"
#include <memory>
#include <stdexcept>
#include <thread>

namespace kolosal
{

    class ServerAPI::Impl
    {
    public:
        std::unique_ptr<Server> server;
        std::unique_ptr<NodeManager> nodeManager;

        Impl()
        {
        }
        
        void initNodeManager(std::chrono::seconds idleTimeout)
        {
            nodeManager = std::make_unique<NodeManager>(idleTimeout);
        }
    };

    ServerAPI::ServerAPI() : pImpl(std::make_unique<Impl>()) {}

    ServerAPI::~ServerAPI()
    {
        shutdown();
    }

    ServerAPI &ServerAPI::instance()
    {
        static ServerAPI instance;
        return instance;
    }
    bool ServerAPI::init(const std::string &port, const std::string &host, std::chrono::seconds idleTimeout)
    {
        try
        {
            ServerLogger::logInfo("Initializing server on %s:%s with idle timeout: %lld seconds", host.c_str(), port.c_str(), idleTimeout.count());

            // Initialize NodeManager with configured idle timeout
            pImpl->initNodeManager(idleTimeout);

            pImpl->server = std::make_unique<Server>(port, host);
            if (!pImpl->server->init())
            {
                ServerLogger::logError("Failed to initialize server");
                return false;
            }
            
            // Register routes
            ServerLogger::logInfo("Registering routes");
            pImpl->server->addRoute(std::make_unique<ChatCompletionsRoute>());
            pImpl->server->addRoute(std::make_unique<CompletionsRoute>());
            pImpl->server->addRoute(std::make_unique<EmbeddingRoute>());
            pImpl->server->addRoute(std::make_unique<ModelsRoute>());
            pImpl->server->addRoute(std::make_unique<InferenceCompletionRoute>());
            pImpl->server->addRoute(std::make_unique<InferenceChatCompletionRoute>());
            pImpl->server->addRoute(std::make_unique<AddEngineRoute>());
            pImpl->server->addRoute(std::make_unique<ListEnginesRoute>());
            pImpl->server->addRoute(std::make_unique<RemoveEngineRoute>());
            pImpl->server->addRoute(std::make_unique<EngineStatusRoute>());
            pImpl->server->addRoute(std::make_unique<HealthStatusRoute>());
            pImpl->server->addRoute(std::make_unique<AuthConfigRoute>());
            pImpl->server->addRoute(std::make_unique<DownloadProgressRoute>());
            pImpl->server->addRoute(std::make_unique<DownloadsStatusRoute>());
            pImpl->server->addRoute(std::make_unique<CancelDownloadRoute>());
            pImpl->server->addRoute(std::make_unique<PauseDownloadRoute>());
            pImpl->server->addRoute(std::make_unique<ResumeDownloadRoute>());
            pImpl->server->addRoute(std::make_unique<CancelAllDownloadsRoute>());
            pImpl->server->addRoute(std::make_unique<ParsePDFRoute>());
            pImpl->server->addRoute(std::make_unique<ParseDOCXRoute>());            
            pImpl->server->addRoute(std::make_unique<AddDocumentsRoute>());
            pImpl->server->addRoute(std::make_unique<kolosal::retrieval::RemoveDocumentsRoute>());
            pImpl->server->addRoute(std::make_unique<RetrieveRoute>());

            ServerLogger::logInfo("Routes registered successfully");

            // Start server in a background thread
            std::thread([this]()
                        {
                ServerLogger::logInfo("Starting server main loop");
                pImpl->server->run(); })
                .detach();

            return true;
        }
        catch (const std::exception &ex)
        {
            ServerLogger::logError("Failed to initialize server: %s", ex.what());
            return false;
        }
    }
    void ServerAPI::shutdown()
    {
        if (pImpl->server)
        {
            ServerLogger::logInfo("Shutting down server");

            // Wait for all download threads to complete (this will cancel them first)
            try
            {
                auto &download_manager = DownloadManager::getInstance();
                ServerLogger::logInfo("Stopping all downloads and waiting for threads to finish...");
                download_manager.waitForAllDownloads();
            }
            catch (const std::exception &ex)
            {
                ServerLogger::logError("Error during download shutdown: %s", ex.what());
            }

            // Shutdown the server
            ServerLogger::logInfo("Shutting down HTTP server");
            pImpl->server.reset();
            ServerLogger::logInfo("Server shutdown complete");
        }
    }

    void ServerAPI::enableMetrics()
    {
        if (!pImpl->server)
        {
            throw std::runtime_error("Server not initialized - call init() first");
        }

        ServerLogger::logInfo("Metrics functionality not yet implemented");
        // TODO: Add metrics routes when they are implemented
        // pImpl->server->addRoute(std::make_unique<SystemMetricsRoute>());
        // pImpl->server->addRoute(std::make_unique<CompletionMetricsRoute>());
    }

    void ServerAPI::enableSearch(const SearchConfig &config)
    {
        if (!pImpl->server)
        {
            throw std::runtime_error("Server not initialized - call init() first");
        }

        ServerLogger::logInfo("Enabling internet search endpoint");
        pImpl->server->addRoute(std::make_unique<InternetSearchRoute>(config));
    }

    NodeManager &ServerAPI::getNodeManager()
    {
        return *pImpl->nodeManager;
    }
    const NodeManager &ServerAPI::getNodeManager() const
    {
        return *pImpl->nodeManager;
    }

    auth::AuthMiddleware &ServerAPI::getAuthMiddleware()
    {
        if (!pImpl->server)
        {
            throw std::runtime_error("Server not initialized");
        }
        return pImpl->server->getAuthMiddleware();
    }

    const auth::AuthMiddleware &ServerAPI::getAuthMiddleware() const
    {
        if (!pImpl->server)
        {
            throw std::runtime_error("Server not initialized");
        }
        return pImpl->server->getAuthMiddleware();
    }

} // namespace kolosal