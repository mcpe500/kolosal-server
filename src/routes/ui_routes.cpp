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
            // Extract the path without query parameters
            std::string cleanPath = path;
            size_t queryPos = path.find('?');
            if (queryPos != std::string::npos) {
                cleanPath = path.substr(0, queryPos);
            }
            
            // Match playground routes
            if (cleanPath == "/playground" || cleanPath == "/playground/") {
                current_method_ = method;
                current_path_ = "/playground/playground.html";
                return true;
            }
            
            // Match direct playground.html access
            if (cleanPath == "/playground/playground.html") {
                current_method_ = method;
                current_path_ = "/playground/playground.html";
                return true;
            }
            
            // Match dashboard routes
            if (cleanPath == "/" || cleanPath == "/dashboard" || cleanPath == "/dashboard/") {
                current_method_ = method;
                current_path_ = "/index.html";
                return true;
            }
            
            // Match specific dashboard pages (both with and without /dashboard prefix)
            if (cleanPath == "/dashboard/index" || cleanPath == "/dashboard/index.html" || 
                cleanPath == "/index" || cleanPath == "/index.html") {
                current_method_ = method;
                current_path_ = "/index.html";
                return true;
            }
            
            if (cleanPath == "/dashboard/engine" || cleanPath == "/dashboard/engine.html" ||
                cleanPath == "/engine" || cleanPath == "/engine.html") {
                current_method_ = method;
                current_path_ = "/engine.html";
                return true;
            }
            
            if (cleanPath == "/dashboard/collection" || cleanPath == "/dashboard/collection.html" ||
                cleanPath == "/collection" || cleanPath == "/collection.html") {
                current_method_ = method;
                current_path_ = "/collection.html";
                return true;
            }
            
            if (cleanPath == "/dashboard/retrieve" || cleanPath == "/dashboard/retrieve.html" ||
                cleanPath == "/retrieve" || cleanPath == "/retrieve.html") {
                current_method_ = method;
                current_path_ = "/retrieve.html";
                return true;
            }
            
            if (cleanPath == "/dashboard/upload" || cleanPath == "/dashboard/upload.html" ||
                cleanPath == "/upload" || cleanPath == "/upload.html") {
                current_method_ = method;
                current_path_ = "/upload.html";
                return true;
            }
            
            // Match static assets (CSS, JS) - with /playground prefix
            if (cleanPath.length() >= 19 && cleanPath.substr(0, 19) == "/playground/styles/" && 
                cleanPath.length() >= 4 && cleanPath.substr(cleanPath.length() - 4) == ".css") {
                current_method_ = method;
                current_path_ = cleanPath; // Keep the full path for playground assets
                return true;
            }
            
            if (cleanPath.length() >= 19 && cleanPath.substr(0, 19) == "/playground/script/" && 
                cleanPath.length() >= 3 && cleanPath.substr(cleanPath.length() - 3) == ".js") {
                current_method_ = method;
                current_path_ = cleanPath; // Keep the full path for playground assets
                return true;
            }
            
            // Match static assets (CSS, JS) - with /dashboard prefix
            if (cleanPath.length() >= 18 && cleanPath.substr(0, 18) == "/dashboard/styles/" && 
                cleanPath.length() >= 4 && cleanPath.substr(cleanPath.length() - 4) == ".css") {
                current_method_ = method;
                current_path_ = cleanPath.substr(10); // Remove "/dashboard" prefix
                return true;
            }
            
            if (cleanPath.length() >= 18 && cleanPath.substr(0, 18) == "/dashboard/script/" && 
                cleanPath.length() >= 3 && cleanPath.substr(cleanPath.length() - 3) == ".js") {
                current_method_ = method;
                current_path_ = cleanPath.substr(10); // Remove "/dashboard" prefix
                return true;
            }
            
            // Match static assets (CSS, JS) - without /dashboard prefix
            if (cleanPath.length() >= 8 && cleanPath.substr(0, 8) == "/styles/" && 
                cleanPath.length() >= 4 && cleanPath.substr(cleanPath.length() - 4) == ".css") {
                current_method_ = method;
                current_path_ = cleanPath; // Use path as is
                return true;
            }
            
            if (cleanPath.length() >= 8 && cleanPath.substr(0, 8) == "/script/" && 
                cleanPath.length() >= 3 && cleanPath.substr(cleanPath.length() - 3) == ".js") {
                current_method_ = method;
                current_path_ = cleanPath; // Use path as is
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
        // Determine which static directory to use based on the path
        std::filesystem::path staticDir;
        std::filesystem::path filePath;
        
        // Get the absolute path to the executable's directory
        std::filesystem::path executablePath = std::filesystem::current_path();
        
        if (relativePath.find("/playground/") == 0) {
            // This is a playground file
            staticDir = executablePath / "static" / "kolosal-playground";
            filePath = relativePath.substr(12); // Remove "/playground/" prefix
        } else {
            // This is a dashboard file
            staticDir = executablePath / "static" / "kolosal-dashboard";
            filePath = relativePath.substr(1); // Remove leading slash
        }
        
        std::filesystem::path fullPath = staticDir / filePath;
        
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
        a { color: #3498db; text-decoration: none; margin: 0 10px; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <h1 class="error">404 - Page Not Found</h1>
    <p class="message">The requested page could not be found.</p>
    <p><a href="/dashboard">Go to Dashboard</a> | <a href="/playground">Go to Playground</a></p>
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