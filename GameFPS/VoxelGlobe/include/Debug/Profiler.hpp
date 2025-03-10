// ./include/Debug/Profiler.hpp
#ifndef PROFILER_HPP
#define PROFILER_HPP

#include <string>
#include <unordered_map>
#include <chrono>
#include <vector>
#include <mutex>
#include <memory>
#include "Debug/Logger.hpp"

/**
 * Simple profiling utility to measure execution time of various game subsystems
 */
class Profiler {
public:
    struct ProfileResult {
        std::string name;
        double startTime;
        double duration;
        LogCategory category;
    };
    
    struct ProfileSection {
        std::string name;
        int hitCount;
        double totalTime;
        double minTime;
        double maxTime;
        double avgTime;
        LogCategory category;
    };
    
    // Get singleton instance
    static Profiler& getInstance();
    
    // Delete copy and move constructors
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    Profiler(Profiler&&) = delete;
    Profiler& operator=(Profiler&&) = delete;
    
    // Start profiling a section
    void beginSection(const std::string& name, LogCategory category = LogCategory::GENERAL);
    
    // End the current section
    void endSection();
    
    // Get profiling results
    const std::vector<ProfileSection>& getResults() const;
    
    // Reset all profiling data
    void reset();
    
    // Enable/disable profiling
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Report profiling results to the log
    void reportResults();
    
    // Set minimum time (in ms) for a section to be reported in logs
    void setReportThreshold(double thresholdMs);
    
private:
    Profiler();
    
    double getTimeSeconds() const;
    
    std::vector<ProfileResult> activeSections;
    std::unordered_map<std::string, ProfileSection> profileSections;
    mutable std::vector<ProfileSection> sortedSections; // Make mutable so it can be sorted in const method
    
    bool enabled;
    double reportThresholdMs;
    mutable std::mutex profilerMutex;
};

// Scoped profiler utility class
class ScopedProfiler {
public:
    ScopedProfiler(const std::string& name, LogCategory category = LogCategory::GENERAL) {
        if (Profiler::getInstance().isEnabled()) {
            name_ = name;
            Profiler::getInstance().beginSection(name, category);
        }
    }
    
    ~ScopedProfiler() {
        if (Profiler::getInstance().isEnabled() && !name_.empty()) {
            Profiler::getInstance().endSection();
        }
    }
    
private:
    std::string name_;
};

// Macros for easy profiling
#ifdef ENABLE_PROFILING
    #define PROFILE_FUNCTION() ScopedProfiler profiler__##__LINE__(__FUNCTION__, LogCategory::GENERAL)
    #define PROFILE_SCOPE(name, category) ScopedProfiler profiler__##__LINE__(name, category)
#else
    #define PROFILE_FUNCTION()
    #define PROFILE_SCOPE(name, category)
#endif

#endif // PROFILER_HPP