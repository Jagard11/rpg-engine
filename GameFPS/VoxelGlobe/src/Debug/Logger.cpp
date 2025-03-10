// ./src/Debug/Logger.cpp
#include "Debug/Logger.hpp"
#include <chrono>
#include <iomanip>

// ConsoleLogSink implementation
void ConsoleLogSink::write(LogLevel level, LogCategory category, const std::string& message) {
    // Set console color based on log level
    const char* colorCode = "\033[0m"; // Reset/default
    switch (level) {
        case LogLevel::TRACE:   colorCode = "\033[90m"; break; // Dark gray
        case LogLevel::DEBUG:   colorCode = "\033[37m"; break; // Light gray
        case LogLevel::INFO:    colorCode = "\033[0m";  break; // Default
        case LogLevel::WARNING: colorCode = "\033[33m"; break; // Yellow
        case LogLevel::ERROR:   colorCode = "\033[31m"; break; // Red
        case LogLevel::FATAL:   colorCode = "\033[1;31m"; break; // Bright red
    }

    std::cout << colorCode << message << "\033[0m" << std::endl;
}

// FileLogSink implementation
FileLogSink::FileLogSink(const std::string& filename) {
    logFile.open(filename, std::ios::out | std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
    }
}

FileLogSink::~FileLogSink() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void FileLogSink::write(LogLevel level, LogCategory category, const std::string& message) {
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}

void FileLogSink::flush() {
    if (logFile.is_open()) {
        logFile.flush();
    }
}

// Logger implementation
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : minLevel(LogLevel::INFO) {
    // Initialize enabled categories
    for (int i = static_cast<int>(LogCategory::GENERAL); 
         i <= static_cast<int>(LogCategory::AUDIO); i++) {
        enabledCategories[static_cast<LogCategory>(i)] = true;
    }
    
    // Add default console sink
    addSink(std::make_shared<ConsoleLogSink>());
}

void Logger::setMinLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(logMutex);
    minLevel = level;
}

LogLevel Logger::getMinLogLevel() const {
    std::lock_guard<std::mutex> lock(logMutex);
    return minLevel;
}

void Logger::setCategoryEnabled(LogCategory category, bool enabled) {
    std::lock_guard<std::mutex> lock(logMutex);
    enabledCategories[category] = enabled;
}

bool Logger::isCategoryEnabled(LogCategory category) const {
    std::lock_guard<std::mutex> lock(logMutex);
    auto it = enabledCategories.find(category);
    return it != enabledCategories.end() && it->second;
}

void Logger::addSink(std::shared_ptr<ILogSink> sink) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (sink) {
        sinks.push_back(sink);
    }
}

void Logger::removeSinks() {
    std::lock_guard<std::mutex> lock(logMutex);
    sinks.clear();
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return oss.str();
}

void Logger::log(LogLevel level, LogCategory category, const std::string& message) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Filter by log level and category
    if (level < minLevel || !enabledCategories[category]) {
        return;
    }
    
    // Format the message
    std::ostringstream formattedMessage;
    formattedMessage << "[" << getTimestamp() << "] "
                     << logLevelToString(level) << " "
                     << logCategoryToString(category) << ": "
                     << message;
    
    // Send to all sinks
    for (auto& sink : sinks) {
        sink->write(level, category, formattedMessage.str());
    }
    
    // Auto-flush on warnings and errors
    if (level >= LogLevel::WARNING) {
        for (auto& sink : sinks) {
            sink->flush();
        }
    }
}

void Logger::trace(LogCategory category, const std::string& message) {
    log(LogLevel::TRACE, category, message);
}

void Logger::debug(LogCategory category, const std::string& message) {
    log(LogLevel::DEBUG, category, message);
}

void Logger::info(LogCategory category, const std::string& message) {
    log(LogLevel::INFO, category, message);
}

void Logger::warning(LogCategory category, const std::string& message) {
    log(LogLevel::WARNING, category, message);
}

void Logger::error(LogCategory category, const std::string& message) {
    log(LogLevel::ERROR, category, message);
}

void Logger::fatal(LogCategory category, const std::string& message) {
    log(LogLevel::FATAL, category, message);
}

std::string Logger::logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO ";
        case LogLevel::WARNING: return "WARN ";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "?????";
    }
}

std::string Logger::logCategoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::GENERAL:   return "GENERAL";
        case LogCategory::WORLD:     return "WORLD  ";
        case LogCategory::PLAYER:    return "PLAYER ";
        case LogCategory::PHYSICS:   return "PHYSICS";
        case LogCategory::RENDERING: return "RENDER ";
        case LogCategory::INPUT:     return "INPUT  ";
        case LogCategory::UI:        return "UI     ";
        case LogCategory::NETWORK:   return "NETWORK";
        case LogCategory::AUDIO:     return "AUDIO  ";
        default:                     return "???????";
    }
}