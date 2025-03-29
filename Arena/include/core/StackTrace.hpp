#pragma once

#include <string>
#include <vector>
#include <deque>
#include <chrono>

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

    /**
     * @brief Records a stack trace with timestamp to memory buffer
     * @param context Optional context message to store with the trace
     */
    static void recordTrace(const std::string& context = "");
    
    /**
     * @brief Dumps all recorded traces to a file
     * @param filename Name of the file to write traces to
     * @return true if successful, false otherwise
     */
    static bool dumpTracesToFile(const std::string& filename);
    
    /**
     * @brief Clears all recorded traces
     */
    static void clearTraces();

private:
    /**
     * @brief Signal handler function for crash signals
     */
    static void signalHandler(int sig);
    
    /**
     * @brief Demangles a C++ symbol name
     */
    static std::string demangle(const std::string& symbol);
    
    /**
     * @brief Structure to store a recorded trace
     */
    struct RecordedTrace {
        std::chrono::system_clock::time_point timestamp;
        std::string context;
        std::vector<std::string> stackFrames;
    };
    
    // Circular buffer for storing the last N traces
    static constexpr size_t MAX_TRACES = 100;
    static std::deque<RecordedTrace> s_recordedTraces;
};

} // namespace Core 