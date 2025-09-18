#include "kolosal/routes/ui_routes.hpp"
#include "kolosal/utils.hpp"
#include "kolosal/logger.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <thread>

namespace kolosal {

    bool UIRoute::match(const std::string &method, const std::string &path) {
        if (method == "GET" || method == "OPTIONS") {
            // Match dashboard routes
            if (path == "/" || path == "/dashboard" || path == "/dashboard/") {
                current_method_ = method;
                current_path_ = "/index.html";
                return true;
            }
            
            // Match specific dashboard pages (both with and without /dashboard prefix)
            if (path == "/dashboard/index" || path == "/dashboard/index.html" || 
                path == "/index" || path == "/index.html") {
                current_method_ = method;
                current_path_ = "/index.html";
                return true;
            }
            
            if (path == "/dashboard/engine" || path == "/dashboard/engine.html" ||
                path == "/engine" || path == "/engine.html") {
                current_method_ = method;
                current_path_ = "/engine.html";
                return true;
            }
            
            if (path == "/dashboard/collection" || path == "/dashboard/collection.html" ||
                path == "/collection" || path == "/collection.html") {
                current_method_ = method;
                current_path_ = "/collection.html";
                return true;
            }
            
            if (path == "/dashboard/retrieve" || path == "/dashboard/retrieve.html" ||
                path == "/retrieve" || path == "/retrieve.html") {
                current_method_ = method;
                current_path_ = "/retrieve.html";
                return true;
            }
            
            if (path == "/dashboard/upload" || path == "/dashboard/upload.html" ||
                path == "/upload" || path == "/upload.html") {
                current_method_ = method;
                current_path_ = "/upload.html";
                return true;
            }
            
            // Match static assets (CSS, JS) - with /dashboard prefix
            if (path.length() >= 18 && path.substr(0, 18) == "/dashboard/styles/" && 
                path.length() >= 4 && path.substr(path.length() - 4) == ".css") {
                current_method_ = method;
                current_path_ = path.substr(10); // Remove "/dashboard" prefix
                return true;
            }
            
            if (path.length() >= 18 && path.substr(0, 18) == "/dashboard/script/" && 
                path.length() >= 3 && path.substr(path.length() - 3) == ".js") {
                current_method_ = method;
                current_path_ = path.substr(10); // Remove "/dashboard" prefix
                return true;
            }
            
            // Match static assets (CSS, JS) - without /dashboard prefix
            if (path.length() >= 8 && path.substr(0, 8) == "/styles/" && 
                path.length() >= 4 && path.substr(path.length() - 4) == ".css") {
                current_method_ = method;
                current_path_ = path; // Use path as is
                return true;
            }
            
            if (path.length() >= 8 && path.substr(0, 8) == "/script/" && 
                path.length() >= 3 && path.substr(path.length() - 3) == ".js") {
                current_method_ = method;
                current_path_ = path; // Use path as is
                return true;
            }
        }
        
        return false;
    }

    void UIRoute::handle(SocketType sock, const std::string &body) {
        try {
            // Handle OPTIONS request for CORS preflight
            if (current_method_ == "OPTIONS") {
                ServerLogger::logDebug("[Thread %u] Handling OPTIONS request for UI endpoint: %s", 
                                     std::this_thread::get_id(), current_path_.c_str());
                
                std::map<std::string, std::string> headers = {
                    {"Content-Type", "text/plain"},
                    {"Access-Control-Allow-Origin", "*"},
                    {"Access-Control-Allow-Methods", "GET, OPTIONS"},
                    {"Access-Control-Allow-Headers", "Content-Type, Authorization, X-API-Key"},
                    {"Access-Control-Max-Age", "86400"} // Cache preflight for 24 hours
                };
                
                send_response(sock, 200, "", headers);
                return;
            }

            ServerLogger::logDebug("[Thread %u] Serving UI file: %s", 
                                 std::this_thread::get_id(), current_path_.c_str());

            // Determine content type
            std::string contentType = getContentType(current_path_);
            
            // Serve the file
            serveStaticFile(sock, current_path_, contentType);
            
        } catch (const std::exception &ex) {
            ServerLogger::logError("[Thread %u] Error serving UI file %s: %s", 
                                 std::this_thread::get_id(), current_path_.c_str(), ex.what());
            serve404(sock);
        }
    }

    std::string UIRoute::getContentType(const std::string& filePath) {
        if (filePath.length() >= 5 && filePath.substr(filePath.length() - 5) == ".html") {
            return "text/html; charset=utf-8";
        } else if (filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".css") {
            return "text/css; charset=utf-8";
        } else if (filePath.length() >= 3 && filePath.substr(filePath.length() - 3) == ".js") {
            return "application/javascript; charset=utf-8";
        } else if (filePath.length() >= 5 && filePath.substr(filePath.length() - 5) == ".json") {
            return "application/json; charset=utf-8";
        } else if (filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".png") {
            return "image/png";
        } else if ((filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".jpg") || 
                   (filePath.length() >= 5 && filePath.substr(filePath.length() - 5) == ".jpeg")) {
            return "image/jpeg";
        } else if (filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".gif") {
            return "image/gif";
        } else if (filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".svg") {
            return "image/svg+xml";
        } else if (filePath.length() >= 4 && filePath.substr(filePath.length() - 4) == ".ico") {
            return "image/x-icon";
        }
        return "text/plain; charset=utf-8";
    }

    std::string UIRoute::readStaticFile(const std::string& relativePath) {
        // Construct the full path to the static file
        std::filesystem::path staticDir = std::filesystem::current_path() / "static" / "kolosal-dashboard";
        std::filesystem::path fullPath = staticDir / relativePath.substr(1); // Remove leading slash
        
        // Security check: ensure the path is within the static directory
        std::filesystem::path canonicalPath = std::filesystem::canonical(fullPath.parent_path()) / fullPath.filename();
        std::filesystem::path canonicalStatic = std::filesystem::canonical(staticDir);
        
        // Check if the canonical path starts with the canonical static directory
        std::string pathStr = canonicalPath.string();
        std::string staticStr = canonicalStatic.string();
        if (pathStr.find(staticStr) != 0) {
            throw std::runtime_error("Path traversal attack detected");
        }
        
        // Read the file
        std::ifstream file(canonicalPath, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("File not found: " + relativePath);
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    void UIRoute::serveStaticFile(SocketType sock, const std::string& filePath, const std::string& contentType) {
        try {
            std::string content = readStaticFile(filePath);
            
            std::map<std::string, std::string> headers = {
                {"Content-Type", contentType},
                {"Access-Control-Allow-Origin", "*"},
                {"Access-Control-Allow-Methods", "GET, OPTIONS"},
                {"Access-Control-Allow-Headers", "Content-Type, Authorization, X-API-Key"},
                {"Cache-Control", "public, max-age=3600"} // Cache for 1 hour
            };
            
            send_response(sock, 200, content, headers);
            
            ServerLogger::logDebug("[Thread %u] Successfully served %s (%zu bytes)", 
                                 std::this_thread::get_id(), filePath.c_str(), content.size());
                                 
        } catch (const std::exception &ex) {
            ServerLogger::logError("[Thread %u] Failed to serve %s: %s", 
                                 std::this_thread::get_id(), filePath.c_str(), ex.what());
            serve404(sock);
        }
    }

    void UIRoute::serve404(SocketType sock) {
        std::string content = R"(<!DOCTYPE html>
<html>
<head>
    <title>404 - Page Not Found</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .error { color: #e74c3c; }
        .message { color: #7f8c8d; margin-top: 20px; }
        a { color: #3498db; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1 class="error">404 - Page Not Found</h1>
    <p class="message">The requested page could not be found.</p>
    <p><a href="/dashboard">Go to Dashboard</a></p>
</body>
</html>)";

        std::map<std::string, std::string> headers = {
            {"Content-Type", "text/html; charset=utf-8"},
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type, Authorization, X-API-Key"}
        };
        
        send_response(sock, 404, content, headers);
    }

} // namespace kolosal