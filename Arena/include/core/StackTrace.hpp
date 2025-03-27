#pragma once

#include <string>
#include <vector>

namespace Core {

/**
 * @brief Utility class for capturing and printing stack traces
 */
class StackTrace {
public:
    /**
     * @brief Installs signal handlers for common crash signals
     */
    static void installSignalHandlers();
    
    /**
     * @brief Captures and returns the current stack trace
     * @param skipFrames Number of frames to skip from the top (usually 1-2 to skip the printStackTrace call itself)
     * @return Vector of strings representing the stack frames
     */
    static std::vector<std::string> captureStackTrace(int skipFrames = 1);
    
    /**
     * @brief Prints the current stack trace to stderr
     * @param skipFrames Number of frames to skip from the top
     */
    static void printStackTrace(int skipFrames = 1);
    
    /**
     * @brief Logs a message with timestamp to a file
     * @param message The message to log
     */
    static void log(const std::string& message);

private:
    /**
     * @brief Signal handler function for crash signals
     */
    static void signalHandler(int sig);
    
    /**
     * @brief Demangles a C++ symbol name
     */
    static std::string demangle(const std::string& symbol);
};

} // namespace Core 