// ./include/Debug/Logger.hpp
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>

/**
 * Enum defining log levels with increasing severity
 */
enum class LogLevel {
    TRACE,   // Very detailed information, for tracing code execution
    DEBUG,   // More detailed information useful for debugging
    INFO,    // Standard informational messages
    WARNING, // Warnings that don't prevent execution but indicate potential issues
    ERROR,   // Errors that affect functionality but don't crash the application
    FATAL    // Critical errors that will likely cause the application to crash
};

/**
 * Enum defining different logging categories/subsystems
 */
enum class LogCategory {
    GENERAL,    // General logs not specific to any category
    WORLD,      // World generation and chunk management
    PLAYER,     // Player movement and interactions
    PHYSICS,    // Physics and collision detection
    RENDERING,  // Rendering and graphics
    INPUT,      // Input handling
    UI,         // User interface
    NETWORK,    // Network communication (future expansion)
    AUDIO       // Audio system (future expansion)
};

/**
 * Abstract log sink interface to allow for multiple outputs
 */
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(LogLevel level, LogCategory category, const std::string& message) = 0;
    virtual void flush() = 0;
};

/**
 * Console output implementation of ILogSink
 */
class ConsoleLogSink : public ILogSink {
public:
    void write(LogLevel level, LogCategory category, const std::string& message) override;
    void flush() override { std::cout.flush(); }
};

/**
 * File output implementation of ILogSink
 */
class FileLogSink : public ILogSink {
public:
    FileLogSink(const std::string& filename);
    ~FileLogSink();
    void write(LogLevel level, LogCategory category, const std::string& message) override;
    void flush() override;

private:
    std::ofstream logFile;
};

/**
 * Main logger class providing a centralized logging system
 * Uses the singleton pattern for global access
 */
class Logger {
public:
    // Get singleton instance
    static Logger& getInstance();

    // Delete copy and move constructors/assignment operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    // Configure the logger
    void setMinLogLevel(LogLevel level);
    LogLevel getMinLogLevel() const;
    void setCategoryEnabled(LogCategory category, bool enabled);
    bool isCategoryEnabled(LogCategory category) const;
    void addSink(std::shared_ptr<ILogSink> sink);
    void removeSinks();

    // Logging methods
    void log(LogLevel level, LogCategory category, const std::string& message);
    
    // Convenience methods for different log levels
    void trace(LogCategory category, const std::string& message);
    void debug(LogCategory category, const std::string& message);
    void info(LogCategory category, const std::string& message);
    void warning(LogCategory category, const std::string& message);
    void error(LogCategory category, const std::string& message);
    void fatal(LogCategory category, const std::string& message);

    // Helper for converting enum values to strings
    static std::string logLevelToString(LogLevel level);
    static std::string logCategoryToString(LogCategory category);

private:
    // Private constructor for singleton
    Logger();

    // Minimum log level to show
    LogLevel minLevel;
    
    // Map of enabled categories
    std::unordered_map<LogCategory, bool> enabledCategories;
    
    // Collection of log sinks
    std::vector<std::shared_ptr<ILogSink>> sinks;
    
    // Mutex for thread safety - mutable so it can be locked in const methods
    mutable std::mutex logMutex;
    
    // Format timestamp
    std::string getTimestamp() const;
};

// Macros for easy logging
#define LOG_TRACE(category, message) Logger::getInstance().trace(category, message)
#define LOG_DEBUG(category, message) Logger::getInstance().debug(category, message)
#define LOG_INFO(category, message) Logger::getInstance().info(category, message)
#define LOG_WARNING(category, message) Logger::getInstance().warning(category, message)
#define LOG_ERROR(category, message) Logger::getInstance().error(category, message)
#define LOG_FATAL(category, message) Logger::getInstance().fatal(category, message)

// Stream-style logging macros
#define LOG_STREAM(level, category) LogStream(level, category)

class LogStream {
public:
    LogStream(LogLevel level, LogCategory category) : level(level), category(category) {}
    ~LogStream() {
        Logger::getInstance().log(level, category, stream.str());
    }

    template <typename T>
    LogStream& operator<<(const T& value) {
        stream << value;
        return *this;
    }

private:
    LogLevel level;
    LogCategory category;
    std::ostringstream stream;
};

#endif // LOGGER_HPP