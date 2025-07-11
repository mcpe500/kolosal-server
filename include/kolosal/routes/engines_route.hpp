#ifndef KOLOSAL_ENGINES_ROUTE_HPP
#define KOLOSAL_ENGINES_ROUTE_HPP

#include "route_interface.hpp"

namespace kolosal
{

    /**
     * @brief Route handler for managing inference engine libraries
     * 
     * Handles GET requests to list available inference engines and
     * POST requests to add new inference engines to the configuration
     * 
     * Supported endpoints:
     * - GET /inference-engines or /v1/inference-engines - List available engines
     * - POST /inference-engines or /v1/inference-engines - Add new engine
     */
    class EnginesRoute : public IRoute
    {
    public:
        /**
         * @brief Check if this route matches the given request
         * @param method HTTP method (GET or POST)
         * @param path Request path (should be /inference-engines or /v1/inference-engines)
         * @return True if this route should handle the request
         */
        bool match(const std::string &method, const std::string &path) override;

        /**
         * @brief Handle the inference engines request
         * @param sock Socket to send response to
         * @param body Request body (used for POST requests to add engines)
         */
        void handle(SocketType sock, const std::string &body) override;

    private:
        // Store the matched method to determine operation in handle()
        mutable std::string matched_method_;
        
        /**
         * @brief Handle GET request to list available inference engines
         * @param sock Socket to send response to
         */
        void handleGetEngines(SocketType sock);

        /**
         * @brief Handle POST request to add a new inference engine
         * @param sock Socket to send response to
         * @param body JSON request body containing engine configuration
         */
        void handleAddEngine(SocketType sock, const std::string &body);
    };

} // namespace kolosal

#endif // KOLOSAL_ENGINES_ROUTE_HPP
