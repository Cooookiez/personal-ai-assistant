//
// Created by Cookiez on 30/03/2025.
//

#ifndef TELEGRAMBOT_H
#define TELEGRAMBOT_H

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <memory>

class TelegramBot;

// Message structure that represents a Telegram message
struct Message {
    int64_t message_id;
    int64_t chat_id;
    std::string from_username;
    std::string text;

    // Parse a message from Telegram API JSON
    static Message fromJson(const nlohmann::json& j);
};

// Update structure representing a Telegram update
struct Update {
    int64_t update_id;
    Message message;

    // Parse an update from Telegram API JSON
    static Update fromJson(const nlohmann::json& j);
};

using CommandHandler = std::function<void(const TelegramBot&, const Message&)>;

// TelegramBot class - Main class for interacting with Telegram API
class TelegramBot {
public:
    explicit TelegramBot(const std::string& token);

    // Start polling for updates
    void start();

    // Stop polling for updates
    void stop();

    // Register a command handler
    void registerCommand(const std::string& command, CommandHandler handler);

    // Send a message to a chat
    void sendMessage(int64_t chat_id, const std::string& text) const;

    // Get bot information
    std::string getBotName() const;

private:
    std::string token_;
    bool running_;
    int64_t last_update_id_;
    std::map<std::string, CommandHandler> command_handlers_;

    // Get updates from Telegram API
    std::vector<Update> getUpdates();

    // Process an update
    void processUpdate(const Update& update);

    // Make a request to Telegram API
    nlohmann::json makeRequest(const std::string& method, const std::map<std::string, std::string>& params = {}) const;
};

#endif //TELEGRAMBOT_H