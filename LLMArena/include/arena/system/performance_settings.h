// include/arena/system/performance_settings.h
#ifndef PERFORMANCE_SETTINGS_H
#define PERFORMANCE_SETTINGS_H

#include <QObject>
#include <QString>
#include <QMap>

/**
 * @brief Manages performance settings for the voxel engine.
 * 
 * This class provides a central place to manage performance settings
 * that can be adjusted at runtime to optimize for different hardware.
 */
class PerformanceSettings : public QObject {
    Q_OBJECT
    
public:
    // Performance preset levels
    enum class Preset {
        Ultra,  // Maximum quality, high-end hardware
        High,   // High quality, modern hardware
        Medium, // Balanced quality, midrange hardware
        Low,    // Reduced quality, older hardware
        Minimal // Minimum quality, very old or integrated hardware
    };
    
    static PerformanceSettings* getInstance();
    
    // Apply a preset to all settings
    void applyPreset(Preset preset);
    
    // Get/set individual settings
    int getViewDistance() const;
    void setViewDistance(int distance);
    
    int getMaxVisibleChunks() const;
    void setMaxVisibleChunks(int maxChunks);
    
    bool isFrustumCullingEnabled() const;
    void setFrustumCullingEnabled(bool enabled);
    
    bool isBackfaceCullingEnabled() const;
    void setBackfaceCullingEnabled(bool enabled);
    
    bool isOcclusionCullingEnabled() const;
    void setOcclusionCullingEnabled(bool enabled);
    
    bool isChunkOptimizationEnabled() const;
    void setChunkOptimizationEnabled(bool enabled);
    
    bool isOctreeCompressionEnabled() const;
    void setOctreeCompressionEnabled(bool enabled);
    
    int getMaxTextureSize() const;
    void setMaxTextureSize(int size);
    
    // Get all current settings as a map
    QMap<QString, QVariant> getAllSettings() const;
    
    // Set a setting by name
    void setSetting(const QString& name, const QVariant& value);
    
signals:
    // Emitted when any setting changes
    void settingsChanged();
    
    // Emitted when a specific setting changes
    void viewDistanceChanged(int distance);
    void maxVisibleChunksChanged(int maxChunks);
    void frustumCullingEnabledChanged(bool enabled);
    void backfaceCullingEnabledChanged(bool enabled);
    void occlusionCullingEnabledChanged(bool enabled);
    void chunkOptimizationEnabledChanged(bool enabled);
    void octreeCompressionEnabledChanged(bool enabled);
    void maxTextureSizeChanged(int size);
    
private:
    // Private constructor - singleton
    explicit PerformanceSettings(QObject* parent = nullptr);
    
    // Settings
    int m_viewDistance;
    int m_maxVisibleChunks;
    bool m_frustumCullingEnabled;
    bool m_backfaceCullingEnabled;
    bool m_occlusionCullingEnabled;
    bool m_chunkOptimizationEnabled;
    bool m_octreeCompressionEnabled;
    int m_maxTextureSize;
    
    // Singleton instance
    static PerformanceSettings* m_instance;
};

#endif // PERFORMANCE_SETTINGS_H