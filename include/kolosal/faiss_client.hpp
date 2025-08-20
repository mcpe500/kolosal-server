#pragma once

#include "export.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <future>
#include <memory>
#include <json.hpp>

namespace kolosal
{

/**
 * @brief Point to be indexed in FAISS
 */
struct FaissPoint
{
    std::string id;
    std::vector<float> vector;
    std::unordered_map<std::string, nlohmann::json> payload;
};

/**
 * @brief Result of FAISS operations
 */
struct FaissResult
{
    bool success = false;
    std::string error_message;
    int status_code = 200;
    
    // For upsert operations
    std::vector<std::string> successful_ids;
    std::vector<std::string> failed_ids;
    
    // For search and other operations that return data
    nlohmann::json response_data;
};

/**
 * @brief FAISS-based vector database client
 * 
 * This client provides thread-safe operations for interacting with FAISS
 * vector database. All operations return futures for non-blocking execution.
 */
class KOLOSAL_SERVER_API FaissClient
{
public:
    /**
     * @brief Configuration for FAISS client
     */
    struct Config
    {
        std::string indexType = "Flat";
        std::string indexPath = "./data/faiss_index";
        int dimensions = 1536;
        bool normalizeVectors = true;
        int nlist = 100;
        int nprobe = 10;
        bool useGPU = false;
        int gpuDevice = 0;
        std::string metricType = "IP"; // L2 or IP (Inner Product)
    };
    
    /**
     * @brief Constructor
     * @param config Client configuration
     */
    explicit FaissClient(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~FaissClient();
    
    // Delete copy constructor and assignment operator
    FaissClient(const FaissClient&) = delete;
    FaissClient& operator=(const FaissClient&) = delete;
    
    // Move constructor and assignment operator
    FaissClient(FaissClient&&) noexcept;
    FaissClient& operator=(FaissClient&&) noexcept;
    
    /**
     * @brief Test connection to FAISS index (initialize if needed)
     * @return Future with result of connection test
     */
    std::future<FaissResult> testConnection();
    
    /**
     * @brief Create a collection if it doesn't exist (initialize index)
     * @param collection_name Name of the collection (used for file path)
     * @param vector_size Size of the vectors (embedding dimensions)
     * @param distance Distance metric (L2, IP)
     * @return Future with result of collection creation
     */
    std::future<FaissResult> createCollection(
        const std::string& collection_name,
        int vector_size,
        const std::string& distance = "IP"
    );
    
    /**
     * @brief Check if collection exists (check if index file exists)
     * @param collection_name Name of the collection
     * @return Future with result (success=true if collection exists)
     */
    std::future<FaissResult> collectionExists(const std::string& collection_name);
    
    /**
     * @brief Upsert points (insert or update) into collection
     * @param collection_name Name of the collection
     * @param points Vector of points to upsert
     * @return Future with result of upsert operation
     */
    std::future<FaissResult> upsertPoints(
        const std::string& collection_name,
        const std::vector<FaissPoint>& points
    );
    
    /**
     * @brief Delete points from collection
     * @param collection_name Name of the collection
     * @param point_ids Vector of point IDs to delete
     * @return Future with result of delete operation
     */
    std::future<FaissResult> deletePoints(
        const std::string& collection_name,
        const std::vector<std::string>& point_ids
    );
    
    /**
     * @brief Get points by their IDs
     * @param collection_name Name of the collection
     * @param point_ids Vector of point IDs to retrieve
     * @return Future with result containing point data
     */
    std::future<FaissResult> getPoints(
        const std::string& collection_name,
        const std::vector<std::string>& point_ids
    );
    
    /**
     * @brief Search for similar vectors
     * @param collection_name Name of the collection
     * @param query_vector Vector to search for
     * @param limit Maximum number of results
     * @param score_threshold Minimum score threshold
     * @return Future with search results
     */
    std::future<FaissResult> search(
        const std::string& collection_name,
        const std::vector<float>& query_vector,
        int limit = 10,
        float score_threshold = 0.0f
    );
    
    /**
     * @brief Scroll through all points in a collection (for listing)
     * @param collection_name Name of the collection
     * @param limit Maximum number of points to retrieve per request
     * @param offset Offset for pagination
     * @return Future with points data
     */
    std::future<FaissResult> scrollPoints(
        const std::string& collection_name,
        int limit = 1000,
        const std::string& offset = ""
    );

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace kolosal
