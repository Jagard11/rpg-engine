// ./src/Debug/DebugManager.cpp
#include "Debug/DebugManager.hpp"
#include "Debug/DebugSystem.hpp"
#include <fstream>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

DebugManager::DebugManager()
    : showVoxelEdges_(false),
      enableCulling(true),  // Changed default to true for better performance
      useFaceColors_(false),
      logPlayerInfo_(false),
      logRaycast_(false),
      logChunkUpdates_(false),
      logBlockPlacement_(false),
      logCollision_(false),
      logInventory_(false),
      debugVertexScaling_(false) {  // Changed default to false since this is fixed now
}

DebugManager& DebugManager::getInstance() {
    static DebugManager instance;
    return instance;
}

void DebugManager::initializeLogging() {
    // Initialize the Logger instance
    auto& logger = Logger::getInstance();
    
    // Set default log level
#ifdef NDEBUG
    logger.setMinLogLevel(LogLevel::INFO);  // Release builds use INFO level
#else
    logger.setMinLogLevel(LogLevel::DEBUG); // Debug builds use DEBUG level
#endif

    // Enable all categories by default
    logger.setCategoryEnabled(LogCategory::GENERAL, true);
    logger.setCategoryEnabled(LogCategory::WORLD, true);
    logger.setCategoryEnabled(LogCategory::PLAYER, true);
    logger.setCategoryEnabled(LogCategory::PHYSICS, true);
    logger.setCategoryEnabled(LogCategory::RENDERING, true);
    logger.setCategoryEnabled(LogCategory::INPUT, true);
    logger.setCategoryEnabled(LogCategory::UI, true);
    
    // Add a file sink for persistent logging
    try {
        auto fileSink = std::make_shared<FileLogSink>("voxel_globe.log");
        logger.addSink(fileSink);
        LOG_INFO(LogCategory::GENERAL, "Logging initialized. Log file created: voxel_globe.log");
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::GENERAL, "Failed to create log file: " + std::string(e.what()));
    }
    
    // Load debug settings if available
    if (loadSettings()) {
        LOG_INFO(LogCategory::GENERAL, "Debug settings loaded from file");
    }
    
    // Log system info
    LOG_INFO(LogCategory::GENERAL, "=== Voxel Globe Debug Manager Initialized ===");
}

// General debug toggles
bool DebugManager::showVoxelEdges() const { 
    return showVoxelEdges_; 
}

bool DebugManager::isCullingEnabled() const { 
    return enableCulling; 
}

bool DebugManager::useFaceColors() const { 
    return useFaceColors_; 
}

// Logging toggles
bool DebugManager::logPlayerInfo() const { 
    return logPlayerInfo_; 
}

bool DebugManager::logRaycast() const { 
    return logRaycast_; 
}

bool DebugManager::logChunkUpdates() const { 
    return logChunkUpdates_; 
}

bool DebugManager::logBlockPlacement() const { 
    return logBlockPlacement_; 
}

bool DebugManager::logCollision() const { 
    return logCollision_; 
}

bool DebugManager::logInventory() const { 
    return logInventory_; 
}

// Mesh debugging flag
bool DebugManager::debugVertexScaling() const { 
    return debugVertexScaling_; 
}

// Setters
void DebugManager::setShowVoxelEdges(bool enabled) { 
    showVoxelEdges_ = enabled; 
    LOG_DEBUG(LogCategory::RENDERING, std::string("Voxel edges ") + (enabled ? "enabled" : "disabled"));
    DebugSystem::getInstance().setShowVoxelEdges(enabled);
    saveSettings();
}

void DebugManager::setCullingEnabled(bool enabled) { 
    enableCulling = enabled; 
    LOG_DEBUG(LogCategory::RENDERING, std::string("Culling ") + (enabled ? "enabled" : "disabled"));
    DebugSystem::getInstance().setCullingEnabled(enabled);
    saveSettings();
}

void DebugManager::setUseFaceColors(bool enabled) { 
    useFaceColors_ = enabled; 
    LOG_DEBUG(LogCategory::RENDERING, std::string("Face colors ") + (enabled ? "enabled" : "disabled"));
    DebugSystem::getInstance().setUseFaceColors(enabled);
    saveSettings();
}

void DebugManager::setLogPlayerInfo(bool enabled) { 
    logPlayerInfo_ = enabled; 
    Logger::getInstance().setCategoryEnabled(LogCategory::PLAYER, enabled);
    LOG_DEBUG(LogCategory::GENERAL, std::string("Player info logging ") + (enabled ? "enabled" : "disabled"));
    saveSettings();
}

void DebugManager::setLogRaycast(bool enabled) { 
    logRaycast_ = enabled; 
    LOG_DEBUG(LogCategory::PHYSICS, std::string("Raycast logging ") + (enabled ? "enabled" : "disabled"));
    saveSettings();
}

void DebugManager::setLogChunkUpdates(bool enabled) { 
    logChunkUpdates_ = enabled; 
    Logger::getInstance().setCategoryEnabled(LogCategory::WORLD, enabled);
    LOG_DEBUG(LogCategory::GENERAL, std::string("Chunk updates logging ") + (enabled ? "enabled" : "disabled"));
    saveSettings();
}

void DebugManager::setLogBlockPlacement(bool enabled) { 
    logBlockPlacement_ = enabled; 
    LOG_DEBUG(LogCategory::WORLD, std::string("Block placement logging ") + (enabled ? "enabled" : "disabled"));
    saveSettings();
}

void DebugManager::setLogCollision(bool enabled) { 
    logCollision_ = enabled; 
    Logger::getInstance().setCategoryEnabled(LogCategory::PHYSICS, enabled);
    LOG_DEBUG(LogCategory::GENERAL, std::string("Collision logging ") + (enabled ? "enabled" : "disabled"));
    saveSettings();
}

void DebugManager::setLogInventory(bool enabled) { 
    logInventory_ = enabled; 
    Logger::getInstance().setCategoryEnabled(LogCategory::UI, enabled);
    LOG_DEBUG(LogCategory::GENERAL, std::string("Inventory logging ") + (enabled ? "enabled" : "disabled"));
    saveSettings();
}

void DebugManager::setDebugVertexScaling(bool enabled) { 
    debugVertexScaling_ = enabled; 
    LOG_DEBUG(LogCategory::RENDERING, std::string("Vertex scaling debug ") + (enabled ? "enabled" : "disabled"));
    DebugSystem::getInstance().setDebugVertexScaling(enabled);
    saveSettings();
}

void DebugManager::setLogLevel(LogLevel level) {
    Logger::getInstance().setMinLogLevel(level);
    
    std::string levelStr;
    switch (level) {
        case LogLevel::TRACE: levelStr = "TRACE"; break;
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO: levelStr = "INFO"; break;
        case LogLevel::WARNING: levelStr = "WARNING"; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::FATAL: levelStr = "FATAL"; break;
    }
    
    LOG_INFO(LogCategory::GENERAL, "Log level set to: " + levelStr);
    saveSettings();
}

LogCategory DebugManager::mapToLogCategory(bool* flag) const {
    if (flag == &logPlayerInfo_) return LogCategory::PLAYER;
    if (flag == &logChunkUpdates_) return LogCategory::WORLD;
    if (flag == &logBlockPlacement_) return LogCategory::WORLD;
    if (flag == &logCollision_) return LogCategory::PHYSICS;
    if (flag == &logRaycast_) return LogCategory::PHYSICS;
    if (flag == &logInventory_) return LogCategory::UI;
    if (flag == &showVoxelEdges_ || flag == &useFaceColors_ || flag == &debugVertexScaling_) 
        return LogCategory::RENDERING;
    
    return LogCategory::GENERAL;
}

void DebugManager::saveSettings(const std::string& filename) const {
    try {
        json settings;
        
        // Save all debug flags
        settings["visualization"] = {
            {"showVoxelEdges", showVoxelEdges_},
            {"enableCulling", enableCulling},
            {"useFaceColors", useFaceColors_},
            {"debugVertexScaling", debugVertexScaling_}
        };
        
        // Save logging flags
        settings["logging"] = {
            {"logPlayerInfo", logPlayerInfo_},
            {"logRaycast", logRaycast_},
            {"logChunkUpdates", logChunkUpdates_},
            {"logBlockPlacement", logBlockPlacement_},
            {"logCollision", logCollision_},
            {"logInventory", logInventory_}
        };
        
        // Save log level
        settings["logLevel"] = static_cast<int>(Logger::getInstance().getMinLogLevel());
        
        // Save category enabled states
        json categories;
        for (int i = static_cast<int>(LogCategory::GENERAL); 
             i <= static_cast<int>(LogCategory::AUDIO); i++) {
            auto category = static_cast<LogCategory>(i);
            categories[Logger::logCategoryToString(category)] = 
                Logger::getInstance().isCategoryEnabled(category);
        }
        settings["logCategories"] = categories;
        
        // Write to file
        std::ofstream file(filename);
        if (file.is_open()) {
            file << settings.dump(4); // Pretty print with 4-space indent
            // No logging here to avoid recursion
        }
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::GENERAL, "Error saving debug settings: " + std::string(e.what()));
    }
}

bool DebugManager::loadSettings(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }
        
        json settings;
        file >> settings;
        
        // Load visualization settings
        if (settings.contains("visualization")) {
            auto& vis = settings["visualization"];
            if (vis.contains("showVoxelEdges")) showVoxelEdges_ = vis["showVoxelEdges"].get<bool>();
            if (vis.contains("enableCulling")) enableCulling = vis["enableCulling"].get<bool>();
            if (vis.contains("useFaceColors")) useFaceColors_ = vis["useFaceColors"].get<bool>();
            if (vis.contains("debugVertexScaling")) debugVertexScaling_ = vis["debugVertexScaling"].get<bool>();
            
            // Sync with DebugSystem
            DebugSystem::getInstance().setShowVoxelEdges(showVoxelEdges_);
            DebugSystem::getInstance().setCullingEnabled(enableCulling);
            DebugSystem::getInstance().setUseFaceColors(useFaceColors_);
            DebugSystem::getInstance().setDebugVertexScaling(debugVertexScaling_);
        }
        
        // Load logging flags
        if (settings.contains("logging")) {
            auto& logging = settings["logging"];
            if (logging.contains("logPlayerInfo")) logPlayerInfo_ = logging["logPlayerInfo"].get<bool>();
            if (logging.contains("logRaycast")) logRaycast_ = logging["logRaycast"].get<bool>();
            if (logging.contains("logChunkUpdates")) logChunkUpdates_ = logging["logChunkUpdates"].get<bool>();
            if (logging.contains("logBlockPlacement")) logBlockPlacement_ = logging["logBlockPlacement"].get<bool>();
            if (logging.contains("logCollision")) logCollision_ = logging["logCollision"].get<bool>();
            if (logging.contains("logInventory")) logInventory_ = logging["logInventory"].get<bool>();
        }
        
        // Load log level
        if (settings.contains("logLevel")) {
            LogLevel level = static_cast<LogLevel>(settings["logLevel"].get<int>());
            Logger::getInstance().setMinLogLevel(level);
        }
        
        // Load category enabled states
        if (settings.contains("logCategories")) {
            auto& categories = settings["logCategories"];
            for (int i = static_cast<int>(LogCategory::GENERAL); 
                i <= static_cast<int>(LogCategory::AUDIO); i++) {
                auto category = static_cast<LogCategory>(i);
                std::string categoryName = Logger::logCategoryToString(category);
                if (categories.contains(categoryName)) {
                    Logger::getInstance().setCategoryEnabled(category, categories[categoryName].get<bool>());
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::GENERAL, "Error loading debug settings: " + std::string(e.what()));
        return false;
    }
}