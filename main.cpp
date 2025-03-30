#include <iostream>
#include <csignal>
#include <fstream>
#include <string>
#include "Models/TelegramBot.h"

// Global bot pointer for signal handling
TelegramBot* g_bot = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    if (g_bot) {
        std::cout << "Stopping bot..." << std::endl;
        g_bot->stop();
    }
}

// Example command handlers
void handleStart(const TelegramBot& bot, const Message& message) {
    bot.sendMessage(message.chat_id, "Hello! I'm a C++ Telegram bot. Use /help to see available commands.");
}

void handleHelp(const TelegramBot& bot, const Message& message) {
    bot.sendMessage(message.chat_id, "Available commands:\n"
                                    "/start - Start the bot\n"
                                    "/help - Show this help message\n"
                                    "/echo [text] - Echo back your text");
}

void handleEcho(const TelegramBot& bot, const Message& message) {
    std::string text = message.text;
    size_t space_pos = text.find(' ');

    if (space_pos != std::string::npos && space_pos + 1 < text.length()) {
        std::string echo_text = text.substr(space_pos + 1);
        bot.sendMessage(message.chat_id, echo_text);
    } else {
        bot.sendMessage(message.chat_id, "Usage: /echo [text]");
    }
}

// Function to read token from env file
std::string readTokenFromEnvFile(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        if (line.find("TELEGRAM_BOT_TOKEN=") != std::string::npos) {
            return line.substr(std::string("TELEGRAM_BOT_TOKEN=").length());
        }
    }

    throw std::runtime_error("Bot token not found in env file");
}

int main() {
    try {
        // Read token from env file
        std::string token = readTokenFromEnvFile("dev.env");

        // Create the bot with the token
        TelegramBot bot(token);
        g_bot = &bot;

        // Set up signal handling for graceful shutdown
        std::signal(SIGINT, signalHandler);

        // Register command handlers
        bot.registerCommand("start", handleStart);
        bot.registerCommand("help", handleHelp);
        bot.registerCommand("echo", handleEcho);

        std::cout << "Bot started. Press Ctrl+C to stop." << std::endl;

        // Start the bot
        bot.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}