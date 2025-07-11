#ifndef KOLOSAL_ENGINES_ROUTE_HPP
#define KOLOSAL_ENGINES_ROUTE_HPP

#include "route_interface.hpp"
#include <regex>

namespace kolosal
{

    /**
     * @brief Route handler for inference engine management
     * 
     * Handles:
     * - GET /inference-engines and /v1/inference-engines (list available engines)
     * - POST /inference-engines and /v1/inference-engines (download/install engine)
     * - GET /inference-engines/{id}/status and /v1/inference-engines/{id}/status (check engine status)
     * - POST /inference-engines/rescan and /v1/inference-engines/rescan (rescan for engines)
     */
    class EnginesRoute : public IRoute
    {
    public:
        /**
         * @brief Check if this route matches the given request
         * @param method HTTP method (GET or POST)
         * @param path Request path
         * @return True if this route should handle the request
         */
        bool match(const std::string &method, const std::string &path) override;

        /**
         * @brief Handle the inference engines request
         * @param sock Socket to send response to
         * @param body Request body
         */
        void handle(SocketType sock, const std::string &body) override;

    private:
        // Static regex patterns for matching routes
        static const std::regex enginesPattern_;
        static const std::regex engineStatusPattern_;
        static const std::regex engineRescanPattern_;

        // Store matched path and method for use in handle()
        std::string matched_path_;
        std::string matched_method_;

        /**
         * @brief Handle listing available inference engines
         * @param sock Socket to send response to
         * @param body Request body (ignored for GET requests)
         */
        void handleListEngines(SocketType sock, const std::string &body);

        /**
         * @brief Handle downloading/installing an inference engine
         * @param sock Socket to send response to
         * @param body Request body containing engine download info
         */
        void handleDownloadEngine(SocketType sock, const std::string &body);

        /**
         * @brief Handle checking engine status
         * @param sock Socket to send response to
         * @param body Request body (ignored for GET requests)
         * @param engineName Engine name to check status for
         */
        void handleEngineStatus(SocketType sock, const std::string &body, const std::string &engineName);

        /**
         * @brief Handle rescanning for inference engines
         * @param sock Socket to send response to
         * @param body Request body (ignored)
         */
        void handleRescanEngines(SocketType sock, const std::string &body);

        /**
         * @brief Extract engine name from path
         * @param path Request path
         * @return Engine name or empty string if not found
         */
        std::string extractEngineIdFromPath(const std::string &path);
    };

} // namespace kolosal

#endif // KOLOSAL_ENGINES_ROUTE_HPP
