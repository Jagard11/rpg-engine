// ./src/Debug/Profiler.cpp
#include "Debug/Profiler.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <GLFW/glfw3.h>

Profiler::Profiler() : enabled(false), reportThresholdMs(1.0) {}

Profiler& Profiler::getInstance() {
    static Profiler instance;
    return instance;
}

void Profiler::beginSection(const std::string& name, LogCategory category) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(profilerMutex);
    
    // Add to active sections
    activeSections.push_back({name, getTimeSeconds(), 0.0, category});
}

void Profiler::endSection() {
    if (!enabled || activeSections.empty()) return;
    
    std::lock_guard<std::mutex> lock(profilerMutex);
    
    // Get the most recently started section
    ProfileResult& result = activeSections.back();
    
    // Calculate duration
    double endTime = getTimeSeconds();
    result.duration = endTime - result.startTime;
    
    // Update statistics in profile sections
    auto it = profileSections.find(result.name);
    if (it == profileSections.end()) {
        // First time seeing this section
        profileSections[result.name] = {
            result.name,
            1,
            result.duration,
            result.duration,
            result.duration,
            result.duration,
            result.category
        };
    } else {
        // Update existing section
        ProfileSection& section = it->second;
        section.hitCount++;
        section.totalTime += result.duration;
        section.minTime = std::min(section.minTime, result.duration);
        section.maxTime = std::max(section.maxTime, result.duration);
        section.avgTime = section.totalTime / section.hitCount;
    }
    
    // Remove the section from active sections
    activeSections.pop_back();
    
    // Log if duration exceeds threshold (converted to ms)
    if (result.duration * 1000.0 > reportThresholdMs) {
        std::stringstream ss;
        ss << "Profiler: " << result.name << " took " 
           << std::fixed << std::setprecision(3) << (result.duration * 1000.0) 
           << " ms";
        LOG_DEBUG(result.category, ss.str());
    }
}

const std::vector<Profiler::ProfileSection>& Profiler::getResults() const {
    std::lock_guard<std::mutex> lock(profilerMutex);
    
    // Sort sections by total time (expensive but only when requested)
    // Using mutable sortedSections to allow modification in const method
    sortedSections.clear();
    
    for (const auto& pair : profileSections) {
        sortedSections.push_back(pair.second);
    }
    
    std::sort(sortedSections.begin(), sortedSections.end(),
        [](const ProfileSection& a, const ProfileSection& b) {
            return a.totalTime > b.totalTime;
        });
    
    return sortedSections;
}

void Profiler::reset() {
    std::lock_guard<std::mutex> lock(profilerMutex);
    
    activeSections.clear();
    profileSections.clear();
    sortedSections.clear();
}

void Profiler::setEnabled(bool enabled_) {
    enabled = enabled_;
    
    if (enabled) {
        LOG_INFO(LogCategory::GENERAL, "Profiler enabled");
    } else {
        LOG_INFO(LogCategory::GENERAL, "Profiler disabled");
    }
}

bool Profiler::isEnabled() const {
    return enabled;
}

void Profiler::reportResults() {
    if (!enabled || profileSections.empty()) return;
    
    std::lock_guard<std::mutex> lock(profilerMutex);
    
    // Get sorted results
    const auto& results = getResults();
    
    LOG_INFO(LogCategory::GENERAL, "===== Profiler Results =====");
    
    std::stringstream ss;
    ss << std::left << std::setw(30) << "Section Name" 
       << std::right << std::setw(10) << "Count" 
       << std::setw(10) << "Total (ms)" 
       << std::setw(10) << "Avg (ms)" 
       << std::setw(10) << "Min (ms)" 
       << std::setw(10) << "Max (ms)";
    LOG_INFO(LogCategory::GENERAL, ss.str());
    
    for (const auto& section : results) {
        ss.str("");
        ss << std::left << std::setw(30) << section.name.substr(0, 30)
           << std::right << std::setw(10) << section.hitCount
           << std::fixed << std::setprecision(2)
           << std::setw(10) << (section.totalTime * 1000.0)
           << std::setw(10) << (section.avgTime * 1000.0)
           << std::setw(10) << (section.minTime * 1000.0)
           << std::setw(10) << (section.maxTime * 1000.0);
        LOG_INFO(LogCategory::GENERAL, ss.str());
    }
    
    LOG_INFO(LogCategory::GENERAL, "============================");
}

void Profiler::setReportThreshold(double thresholdMs) {
    reportThresholdMs = thresholdMs;
}

double Profiler::getTimeSeconds() const {
    return glfwGetTime();
}