#include "core/StackTrace.hpp"

#include <execinfo.h>   // for backtrace() and backtrace_symbols()
#include <cxxabi.h>     // for __cxxabiv1::__cxa_demangle
#include <signal.h>     // for signal handling
#include <unistd.h>     // for STDERR_FILENO
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <filesystem>

namespace Core {

// Initialize static member
std::deque<StackTrace::RecordedTrace> StackTrace::s_recordedTraces;

void StackTrace::installSignalHandlers() {
    signal(SIGSEGV, signalHandler); // Segmentation fault
    signal(SIGABRT, signalHandler); // Abort
    signal(SIGFPE, signalHandler);  // Floating point exception
    signal(SIGILL, signalHandler);  // Illegal instruction
    signal(SIGBUS, signalHandler);  // Bus error
    
    std::cout << "Stack trace signal handlers installed" << std::endl;
}

std::vector<std::string> StackTrace::captureStackTrace(int skipFrames) {
    static const int MAX_FRAMES = 64;
    void* addrlist[MAX_FRAMES + 1];

    // Retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void*));
    if (addrlen == 0) {
        return {"<empty, possibly corrupt stack>"};
    }

    // Convert addresses into an array of strings that describe the addresses symbolically
    char** symbollist = backtrace_symbols(addrlist, addrlen);
    if (symbollist == nullptr) {
        return {"<error: could not generate stack trace>"};
    }

    std::vector<std::string> result;
    result.reserve(addrlen - skipFrames);

    // For each symbol, try to demangle and format it
    for (int i = skipFrames; i < addrlen; i++) {
        std::string symbol(symbollist[i]);
        std::size_t begin = symbol.find('(');
        std::size_t end = symbol.find('+', begin);

        std::stringstream ss;
        ss << "[" << (i - skipFrames) << "] ";

        if (begin != std::string::npos && end != std::string::npos) {
            std::string mangled = symbol.substr(begin + 1, end - (begin + 1));
            std::string demangled = demangle(mangled);
            
            // Extract just the binary/library name from the full path
            std::string binaryPath = symbol.substr(0, begin);
            std::size_t lastSlash = binaryPath.find_last_of('/');
            std::string binary = (lastSlash != std::string::npos) ? 
                binaryPath.substr(lastSlash + 1) : binaryPath;
            
            ss << binary << " - " << demangled;
            
            // Add the address offset if present
            if (end + 1 < symbol.size()) {
                std::size_t addrEnd = symbol.find(')', end);
                if (addrEnd != std::string::npos) {
                    ss << " " << symbol.substr(end, addrEnd - end + 1);
                }
            }
        } else {
            // If demangling fails, just print the symbol
            ss << symbollist[i];
        }

        result.push_back(ss.str());
    }

    std::free(symbollist);
    return result;
}

void StackTrace::recordTrace(const std::string& context) {
    RecordedTrace trace;
    trace.timestamp = std::chrono::system_clock::now();
    trace.context = context;
    trace.stackFrames = captureStackTrace(2); // Skip recordTrace and caller
    
    // Add to the deque, maintaining maximum size
    s_recordedTraces.push_back(std::move(trace));
    if (s_recordedTraces.size() > MAX_TRACES) {
        s_recordedTraces.pop_front();
    }
    
    // Log to stderr that a trace was recorded
    auto time_t_value = std::chrono::system_clock::to_time_t(trace.timestamp);
    std::cerr << "Stack trace recorded at " 
              << std::put_time(std::localtime(&time_t_value), "%Y-%m-%d %H:%M:%S") 
              << " - Context: " << context << std::endl;
}

bool StackTrace::dumpTracesToFile(const std::string& filename) {
    // Create directory if it doesn't exist
    std::filesystem::path filepath(filename);
    std::filesystem::create_directories(filepath.parent_path());
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return false;
    }
    
    auto now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    
    file << "===== Stack Trace Dump =====" << std::endl;
    file << "Dump created: " 
         << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S") 
         << std::endl;
    file << "Number of traces: " << s_recordedTraces.size() << std::endl;
    file << std::endl;
    
    for (size_t i = 0; i < s_recordedTraces.size(); ++i) {
        const auto& trace = s_recordedTraces[i];
        
        // Format timestamp
        auto time_t = std::chrono::system_clock::to_time_t(trace.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            trace.timestamp.time_since_epoch()) % 1000;
        
        file << "----- Trace " << (i + 1) << " -----" << std::endl;
        file << "Time: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") 
             << "." << std::setfill('0') << std::setw(3) << ms.count() << std::endl;
        file << "Context: " << trace.context << std::endl;
        file << "Stack frames:" << std::endl;
        
        for (const auto& frame : trace.stackFrames) {
            file << "  " << frame << std::endl;
        }
        
        file << std::endl;
    }
    
    file << "===== End of Stack Trace Dump =====" << std::endl;
    
    std::cout << "Stack traces dumped to " << filename << std::endl;
    return true;
}

void StackTrace::clearTraces() {
    s_recordedTraces.clear();
    std::cout << "Stack trace buffer cleared" << std::endl;
}

void StackTrace::printStackTrace(int skipFrames) {
    std::cerr << "\n=== Stack Trace ===\n";
    
    auto trace = captureStackTrace(skipFrames + 1);  // +1 to skip printStackTrace itself
    for (const auto& frame : trace) {
        std::cerr << frame << "\n";
    }
    
    std::cerr << "===================\n";
}

void StackTrace::signalHandler(int sig) {
    std::cerr << "\n\n=== Caught signal " << sig << " (";
    
    switch (sig) {
        case SIGSEGV: std::cerr << "Segmentation fault"; break;
        case SIGABRT: std::cerr << "Abort"; break;
        case SIGFPE:  std::cerr << "Floating point exception"; break;
        case SIGILL:  std::cerr << "Illegal instruction"; break;
        case SIGBUS:  std::cerr << "Bus error"; break;
        default:      std::cerr << "Unknown signal"; break;
    }
    
    std::cerr << ") ===\n";
    printStackTrace(1);  // Skip the signal handler frame
    
    // Restore default handler and re-raise signal
    // This allows core dump generation if enabled
    signal(sig, SIG_DFL);
    raise(sig);
}

std::string StackTrace::demangle(const std::string& symbol) {
    int status = 0;
    char* demangled = abi::__cxa_demangle(symbol.c_str(), nullptr, nullptr, &status);
    
    std::string result;
    if (demangled != nullptr && status == 0) {
        result = demangled;
        std::free(demangled);
    } else {
        // If demangling fails, return the original symbol
        result = symbol;
    }
    
    return result;
}

void StackTrace::log(const std::string& message) {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count()
       << " | " << message;
    
    // Open the log file in append mode
    std::ofstream logFile("debug.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << ss.str() << std::endl;
        logFile.close();
    }
    
    // Also print to stderr for immediate feedback
    std::cerr << ss.str() << std::endl;
}

} // namespace Core 