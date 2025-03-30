//
// Created by Cookiez on 30/03/2025.
//

#include "TelegramBot.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// For convenience
using json = nlohmann::json;

// Helper function for curl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
        return newLength;
    } catch(std::bad_alloc& e) {
        // Handle memory problem
        return 0;
    }
}

// Implementation of Message::fromJson
Message Message::fromJson(const json& j) {
    Message msg;
    msg.message_id = j["message_id"];
    msg.chat_id = j["chat"]["id"];

    if (j.contains("from") && j["from"].contains("username")) {
        msg.from_username = j["from"]["username"];
    }

    if (j.contains("text")) {
        msg.text = j["text"];
    }

    return msg;
}

// Implementation of Update::fromJson
Update Update::fromJson(const json& j) {
    Update update;
    update.update_id = j["update_id"];

    if (j.contains("message")) {
        update.message = Message::fromJson(j["message"]);
    }

    return update;
}

// TelegramBot constructor
TelegramBot::TelegramBot(const std::string& token)
    : token_(token), running_(false), last_update_id_(0) {
    // Initialize curl globally
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Get bot info to validate token
    try {
        json me = makeRequest("getMe");
        std::cout << "Connected to Telegram bot: @" << me["result"]["username"].get<std::string>() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to Telegram API: " << e.what() << std::endl;
        throw;
    }
}

// Start polling for updates
void TelegramBot::start() {
    if (running_) return;

    running_ = true;

    while (running_) {
        try {
            // Get updates from Telegram
            std::vector<Update> updates = getUpdates();

            // Process each update
            for (const auto& update : updates) {
                processUpdate(update);
                last_update_id_ = std::max(last_update_id_, update.update_id + 1);
            }
        } catch (const std::exception& e) {
            std::cerr << "Error while processing updates: " << e.what() << std::endl;
        }

        // Sleep to avoid flooding the Telegram servers
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// Stop polling for updates
void TelegramBot::stop() {
    running_ = false;
}

// Register a command handler
void TelegramBot::registerCommand(const std::string& command, CommandHandler handler) {
    command_handlers_[command] = handler;
}

// Send a message to a chat
void TelegramBot::sendMessage(int64_t chat_id, const std::string& text) const {
    std::map<std::string, std::string> params;
    params["chat_id"] = std::to_string(chat_id);
    params["text"] = text;

    makeRequest("sendMessage", params);
}

// Get bot information
std::string TelegramBot::getBotName() const {
    json me = makeRequest("getMe");
    return me["result"]["username"];
}

// Get updates from Telegram API
std::vector<Update> TelegramBot::getUpdates() {
    std::map<std::string, std::string> params;
    params["offset"] = std::to_string(last_update_id_);
    params["timeout"] = "30";

    json response = makeRequest("getUpdates", params);
    std::vector<Update> updates;

    if (response["ok"] && response["result"].is_array()) {
        for (const auto& update_json : response["result"]) {
            updates.push_back(Update::fromJson(update_json));
        }
    }

    return updates;
}

// Process an update
void TelegramBot::processUpdate(const Update& update) {
    const Message& message = update.message;

    // Check if it's a command (starts with '/')
    if (!message.text.empty() && message.text[0] == '/') {
        std::string command = message.text.substr(1);
        size_t space_pos = command.find(' ');
        if (space_pos != std::string::npos) {
            command = command.substr(0, space_pos);
        }

        // Find and execute command handler
        auto it = command_handlers_.find(command);
        if (it != command_handlers_.end()) {
            it->second(*this, message);
        } else {
            sendMessage(message.chat_id, "Unknown command: /" + command);
        }
    }
}

// Make a request to Telegram API
json TelegramBot::makeRequest(const std::string& method, const std::map<std::string, std::string>& params) const {
    CURL* curl = curl_easy_init();
    std::string response_string;

    if (curl) {
        std::string url = "https://api.telegram.org/bot" + token_ + "/" + method;

        // Prepare POST data if we have parameters
        std::string post_fields;
        for (const auto& param : params) {
            if (!post_fields.empty()) {
                post_fields += "&";
            }
            post_fields += param.first + "=" + curl_easy_escape(curl, param.second.c_str(), param.second.length());
        }

        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

        if (!post_fields.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());
        }

        // Perform the request
        CURLcode res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)));
        }

        // Clean up
        curl_easy_cleanup(curl);
    } else {
        throw std::runtime_error("Failed to initialize curl");
    }

    // Parse JSON response
    try {
        json response = json::parse(response_string);
        return response;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse response: " + std::string(e.what()));
    }
}