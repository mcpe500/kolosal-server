#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "export.hpp"
#include "auth/rate_limiter.hpp"
#include "auth/cors_handler.hpp"
#include "inference.h"

namespace kolosal {

/**
 * @brief Configuration for a model to be loaded at startup
 */
struct ModelConfig {
    std::string id;                    // Unique identifier for the model
    std::string path;                  // Path to the model file
    LoadingParameters loadParams;      // Model loading parameters
    int mainGpuId = 0;                // GPU ID to use for this model
    bool loadImmediately = true;      // Whether to load immediately (true) vs lazy load on first use (false)
    std::string inferenceEngine = "llama-cpu"; // Inference engine to use (llama-cpu, llama-cuda, llama-vulkan, etc.)
    
    ModelConfig() = default;
    ModelConfig(const std::string& modelId, const std::string& modelPath, bool load = true)
        : id(modelId), path(modelPath), loadImmediately(load) {}
};

/**
 * @brief Authentication configuration
 */
struct AuthConfig {
    // Rate limiting configuration
    auth::RateLimiter::Config rateLimiter;
    
    // CORS configuration
    auth::CorsHandler::Config cors;
    
    // General auth settings
    bool enableAuth = true;           // Whether authentication is enabled
    bool requireApiKey = false;       // Whether API key is required
    std::string apiKeyHeader = "X-API-Key"; // Header name for API key
    std::vector<std::string> allowedApiKeys; // List of valid API keys
    
    AuthConfig() = default;
};

/**
 * @brief Server startup configuration
 */
struct KOLOSAL_SERVER_API ServerConfig {    // Basic server settings
#pragma warning(push)
#pragma warning(disable: 4251)
    std::string port = "8080";
    std::string host = "0.0.0.0";
#pragma warning(pop)
    bool allowPublicAccess = false;    // Enable/disable external network access
    bool allowInternetAccess = false;  // Enable/disable internet access (UPnP + public IP detection)
      // Logging configuration
#pragma warning(push)
#pragma warning(disable: 4251)
    std::string logLevel = "INFO";    // DEBUG, INFO, WARN, ERROR
    std::string logFile = "";         // Empty means console only
#pragma warning(pop)
    bool enableAccessLog = false;     // Whether to log all requests
    bool quietMode = false;           // Suppress routine operational messages
    bool showRequestDetails = true;   // Show detailed request processing logs
    
    // Performance settings
#pragma warning(push)
#pragma warning(disable: 4251)
    std::chrono::seconds idleTimeout{300}; // Model idle timeout
    
    // Models to load at startup
    std::vector<ModelConfig> models;
#pragma warning(pop)
    
    // Authentication configuration
    AuthConfig auth;
      // Feature flags
    bool enableHealthCheck = true;
    bool enableMetrics = false;
    
    // Internal flags
    bool helpOrVersionShown = false;  // Tracks if help/version was displayed
    
    ServerConfig() = default;
    
    /**
     * @brief Get the global server config instance
     * @return Reference to the global server config
     */
    static ServerConfig& getInstance();
    
    /**
     * @brief Set the global server config instance
     * @param config The config to set as the global instance
     */
    static void setInstance(const ServerConfig& config);
    
    /**
     * @brief Load configuration from command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return True if configuration was loaded successfully
     */
    bool loadFromArgs(int argc, char* argv[]);
      /**
     * @brief Load configuration from YAML file
     * @param configFile Path to configuration file
     * @return True if configuration was loaded successfully
     */
    bool loadFromFile(const std::string& configFile);
    
    /**
     * @brief Save current configuration to YAML file
     * @param configFile Path to save configuration
     * @return True if configuration was saved successfully
     */
    bool saveToFile(const std::string& configFile) const;
    
    /**
     * @brief Validate the configuration
     * @return True if configuration is valid
     */
    bool validate() const;
      /**
     * @brief Print configuration summary
     */
    void printSummary() const;
    
    /**
     * @brief Print help message
     */
    static void printHelp();
    
    /**
     * @brief Print version information
     */
    static void printVersion();
};

} // namespace kolosal
