#pragma once

#include "../export.hpp"
#include "model_interface.hpp"
#include <json.hpp>
#include <sstream>
#include <string>

class KOLOSAL_SERVER_API ChatMessage : public IModel {
public:
    std::string role;
    std::string content;

    bool validate() const override {
        return !role.empty();
    }

    void from_json(const nlohmann::json& j) override {
        if (!j.is_object()) {
            throw std::runtime_error("Message must be a JSON object");
        }

        if (!j.contains("role")) {
            throw std::runtime_error("Message must have a role field");
        }

        if (!j["role"].is_string()) {
            throw std::runtime_error("Role must be a string");
        }

        j.at("role").get_to(role);

        // Content can be null in some cases (like function calls), but we're
        // ignoring those for now
        content = ""; // Default to empty
        if (j.contains("content") && !j["content"].is_null()) {
            content = extractContent(j["content"]);
        }
    }

    nlohmann::json to_json() const override {
        return nlohmann::json{ {"role", role}, {"content", content} };
    }

    static std::string extractContent(const nlohmann::json& value) {
        if (value.is_null()) {
            return "";
        }

        if (value.is_string()) {
            return value.get<std::string>();
        }

        if (value.is_boolean()) {
            return value.get<bool>() ? "true" : "false";
        }

        if (value.is_number()) {
            return value.dump();
        }

        if (value.is_array()) {
            std::ostringstream builder;
            bool appended = false;
            for (const auto& part : value) {
                const std::string text = extractContent(part);
                if (text.empty()) {
                    continue;
                }
                if (appended) {
                    builder << "\n";
                }
                builder << text;
                appended = true;
            }
            return builder.str();
        }

        if (value.is_object()) {
            const auto textIt = value.find("text");
            if (textIt != value.end() && textIt->is_string()) {
                return textIt->get<std::string>();
            }

            const auto contentIt = value.find("content");
            if (contentIt != value.end()) {
                const auto contentValue = *contentIt;
                if (contentValue.is_string()) {
                    return contentValue.get<std::string>();
                }
                if (contentValue.is_array() || contentValue.is_object()) {
                    return extractContent(contentValue);
                }
            }

            const auto partsIt = value.find("parts");
            if (partsIt != value.end() && partsIt->is_array()) {
                return extractContent(*partsIt);
            }

            const auto typeIt = value.find("type");
            if (typeIt != value.end() && typeIt->is_string()) {
                const std::string type = typeIt->get<std::string>();
                if ((type == "text" || type == "input_text" || type == "output_text") &&
                    textIt != value.end() && textIt->is_string()) {
                    return textIt->get<std::string>();
                }

                if (type == "tool_result") {
                    const auto outputTextIt = value.find("output_text");
                    if (outputTextIt != value.end()) {
                        return extractContent(*outputTextIt);
                    }
                }

                if (type == "image_url" || type == "input_image" || type == "image") {
                    return "[image content omitted]";
                }
            }

            const auto imageUrlIt = value.find("image_url");
            if (imageUrlIt != value.end()) {
                return "[image content omitted]";
            }

            // Fallback: include minimal serialization to avoid throwing
            return value.dump();
        }

        // Unknown type - fall back to JSON serialization
        return value.dump();
    }
};
