// ./src/Debug/DebugSystem.cpp
#include "Debug/DebugSystem.hpp"
#include "Utils/SphereUtils.hpp"
#include <GLFW/glfw3.h>
#include <sstream>
#include <iomanip>
#include "../../third_party/nlohmann/json.hpp"

using json = nlohmann::json;

DebugSystem& DebugSystem::getInstance() {
    static DebugSystem instance;
    return instance;
}

DebugSystem::DebugSystem()
    : showVoxelEdges_(false),
      enableCulling_(true),
      useFaceColors_(false),
      debugVertexScaling_(false),
      frameTimeSum(0.0),
      frameCount(0),
      lastReportTime(0.0) {
}

void DebugSystem::initialize() {
    // Configure logger
    Logger::getInstance().setMinLogLevel(LogLevel::DEBUG);
    Logger::getInstance().setCategoryEnabled(LogCategory::GENERAL, true);
    Logger::getInstance().setCategoryEnabled(LogCategory::WORLD, true);
    Logger::getInstance().setCategoryEnabled(LogCategory::PLAYER, true);
    Logger::getInstance().setCategoryEnabled(LogCategory::PHYSICS, true);
    Logger::getInstance().setCategoryEnabled(LogCategory::RENDERING, true);
    Logger::getInstance().setCategoryEnabled(LogCategory::INPUT, true);
    Logger::getInstance().setCategoryEnabled(LogCategory::UI, true);
    
    // Add file sink for persistent logging
    auto fileSink = std::make_shared<FileLogSink>("game.log");
    Logger::getInstance().addSink(fileSink);
    
    // Load debug settings from file
    if (loadSettings()) {
        LOG_INFO(LogCategory::GENERAL, "Debug settings loaded from file");
    } else {
        LOG_INFO(LogCategory::GENERAL, "Using default debug settings");
    }
    
    LOG_INFO(LogCategory::GENERAL, "DebugSystem initialized");
}

void DebugSystem::setShowVoxelEdges(bool enabled) {
    showVoxelEdges_ = enabled;
    LOG_DEBUG(LogCategory::RENDERING, std::string("Voxel edges ") + (enabled ? "enabled" : "disabled"));
}

void DebugSystem::setCullingEnabled(bool enabled) {
    enableCulling_ = enabled;
    LOG_DEBUG(LogCategory::RENDERING, std::string("Culling ") + (enabled ? "enabled" : "disabled"));
}

void DebugSystem::setUseFaceColors(bool enabled) {
    useFaceColors_ = enabled;
    LOG_DEBUG(LogCategory::RENDERING, std::string("Face colors ") + (enabled ? "enabled" : "disabled"));
}

void DebugSystem::setDebugVertexScaling(bool enabled) {
    debugVertexScaling_ = enabled;
    LOG_DEBUG(LogCategory::RENDERING, std::string("Vertex scaling debug ") + (enabled ? "enabled" : "disabled"));
}

bool DebugSystem::showVoxelEdges() const {
    return showVoxelEdges_;
}

bool DebugSystem::isCullingEnabled() const {
    return enableCulling_;
}

bool DebugSystem::useFaceColors() const {
    return useFaceColors_;
}

bool DebugSystem::debugVertexScaling() const {
    return debugVertexScaling_;
}

void DebugSystem::logCoordinateInfo(const std::string& prefix, const glm::vec3& position) {
    std::ostringstream oss;
    
    oss << prefix << " position: " << position.x << ", " << position.y << ", " << position.z;
    LOG_DEBUG(LogCategory::PLAYER, oss.str());
    
    oss.str("");
    float distFromCenter = glm::length(position);
    float heightAboveSurface = distFromCenter - SphereUtils::getSurfaceRadiusMeters();
    
    oss << prefix << " distance from center: " << distFromCenter
        << ", height above surface: " << heightAboveSurface;
    LOG_DEBUG(LogCategory::PLAYER, oss.str());
    
    // Get chunk coordinates
    int chunkX = static_cast<int>(floor(position.x / 16.0f));
    int chunkY = static_cast<int>(floor(position.y / 16.0f));
    int chunkZ = static_cast<int>(floor(position.z / 16.0f));
    
    oss.str("");
    oss << prefix << " chunk coords: (" << chunkX << ", " << chunkY << ", " << chunkZ << ")";
    LOG_DEBUG(LogCategory::PLAYER, oss.str());
}

void DebugSystem::logBlockInfo(const std::string& prefix, int x, int y, int z, int blockType) {
    std::ostringstream oss;
    
    // Calculate chunk coordinates
    int chunkX = static_cast<int>(floor(x / 16.0f));
    int chunkY = static_cast<int>(floor(y / 16.0f));
    int chunkZ = static_cast<int>(floor(z / 16.0f));
    
    // Calculate local block position
    int localX = x - chunkX * 16;
    int localY = y - chunkY * 16;
    int localZ = z - chunkZ * 16;
    
    oss << prefix << " block at world (" << x << ", " << y << ", " << z 
        << ") -> chunk (" << chunkX << ", " << chunkY << ", " << chunkZ 
        << ") local (" << localX << ", " << localY << ", " << localZ 
        << ") type: " << blockType;
    LOG_DEBUG(LogCategory::WORLD, oss.str());
                  
    // Calculate distance from center
    float distFromCenter = glm::length(glm::vec3(x, y, z));
    float surfaceR = SphereUtils::getSurfaceRadiusMeters();
    
    oss.str("");
    oss << prefix << " block distance from center: " << distFromCenter 
        << ", height vs surface: " << (distFromCenter - surfaceR);
    LOG_DEBUG(LogCategory::WORLD, oss.str());
}

void DebugSystem::logCollisionCheck(const std::string& prefix, const glm::vec3& position, bool collided) {
    std::ostringstream oss;
    
    oss << prefix << " collision check at " << position.x << ", " << position.y << ", " << position.z;
    LOG_DEBUG(LogCategory::PHYSICS, oss.str());
    
    float surfaceR = SphereUtils::getSurfaceRadiusMeters();
    float distFromCenter = glm::length(position);
    
    oss.str("");
    oss << "Distance from center: " << distFromCenter 
        << ", surface at: " << surfaceR 
        << ", result: " << (collided ? "COLLISION" : "NO COLLISION");
    LOG_DEBUG(LogCategory::PHYSICS, oss.str());
}

void DebugSystem::beginFrameTiming() {
    lastReportTime = glfwGetTime();
}

void DebugSystem::endFrameTiming() {
    double currentTime = glfwGetTime();
    double frameTime = currentTime - lastReportTime;
    
    frameTimeSum += frameTime;
    frameCount++;
    
    // Report performance every 60 frames
    if (frameCount >= 60) {
        reportPerformance();
    }
}

void DebugSystem::reportPerformance() {
    if (frameCount == 0) return;
    
    double averageFrameTime = frameTimeSum / frameCount;
    double fps = 1.0 / averageFrameTime;
    
    std::ostringstream oss;
    oss << "Performance: " << std::fixed << std::setprecision(2) << fps << " FPS ("
        << (averageFrameTime * 1000.0) << " ms/frame)";
    LOG_INFO(LogCategory::GENERAL, oss.str());
    
    // Reset counters
    frameTimeSum = 0.0;
    frameCount = 0;
}

double DebugSystem::getTimestamp() const {
    return glfwGetTime();
}

void DebugSystem::syncWithDebugManager(const DebugManager& manager) {
    setShowVoxelEdges(manager.showVoxelEdges());
    setCullingEnabled(manager.isCullingEnabled());
    setUseFaceColors(manager.useFaceColors());
    setDebugVertexScaling(manager.debugVertexScaling());
    
    // Save settings whenever they change
    saveSettings();
}

void DebugSystem::saveSettings(const std::string& filename) const {
    try {
        json settings;
        
        // Save visualization settings
        settings["visualization"] = {
            {"showVoxelEdges", showVoxelEdges_},
            {"enableCulling", enableCulling_},
            {"useFaceColors", useFaceColors_},
            {"debugVertexScaling", debugVertexScaling_}
        };
        
        // Save logger settings
        settings["logger"] = {
            {"minLogLevel", static_cast<int>(Logger::getInstance().getMinLogLevel())}
        };
        
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
            LOG_DEBUG(LogCategory::GENERAL, "Debug settings saved to " + filename);
        } else {
            LOG_ERROR(LogCategory::GENERAL, "Failed to save debug settings to " + filename);
        }
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::GENERAL, "Error saving debug settings: " + std::string(e.what()));
    }
}

bool DebugSystem::loadSettings(const std::string& filename) {
    try {
        #include "../../third_party/nlohmann/json.hpp"
        using json = nlohmann::json;
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            LOG_WARNING(LogCategory::GENERAL, "Could not open debug settings file: " + filename);
            return false;
        }
        
        json settings;
        file >> settings;
        
        // Load visualization settings
        if (settings.contains("visualization")) {
            auto& vis = settings["visualization"];
            if (vis.contains("showVoxelEdges")) setShowVoxelEdges(vis["showVoxelEdges"].get<bool>());
            if (vis.contains("enableCulling")) setCullingEnabled(vis["enableCulling"].get<bool>());
            if (vis.contains("useFaceColors")) setUseFaceColors(vis["useFaceColors"].get<bool>());
            if (vis.contains("debugVertexScaling")) setDebugVertexScaling(vis["debugVertexScaling"].get<bool>());
        }
        
        // Load logger settings
        if (settings.contains("logger")) {
            auto& logger = settings["logger"];
                            if (logger.contains("minLogLevel")) {
                LogLevel level = static_cast<LogLevel>(logger["minLogLevel"].get<int>());
                Logger::getInstance().setMinLogLevel(level);
            }
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
        
        LOG_DEBUG(LogCategory::GENERAL, "Debug settings loaded from " + filename);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(LogCategory::GENERAL, "Error loading debug settings: " + std::string(e.what()));
        return false;
    }
}