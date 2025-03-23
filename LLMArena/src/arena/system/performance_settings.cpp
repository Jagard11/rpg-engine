// src/arena/system/performance_settings.cpp
#include "../../../include/arena/system/performance_settings.h"
#include <QVariant>
#include <QDebug>

// Initialize static instance
PerformanceSettings* PerformanceSettings::m_instance = nullptr;

PerformanceSettings* PerformanceSettings::getInstance() {
    if (!m_instance) {
        m_instance = new PerformanceSettings();
    }
    return m_instance;
}

PerformanceSettings::PerformanceSettings(QObject* parent)
    : QObject(parent) {
    // Apply minimal settings by default
    applyPreset(Preset::Minimal);
}

void PerformanceSettings::applyPreset(Preset preset) {
    switch (preset) {
        case Preset::Ultra:
            m_viewDistance = 16;
            m_maxVisibleChunks = 1024;
            m_frustumCullingEnabled = true;
            m_backfaceCullingEnabled = true;
            m_chunkOptimizationEnabled = true;
            m_octreeCompressionEnabled = true;
            m_maxTextureSize = 1024;
            break;
            
        case Preset::High:
            m_viewDistance = 12;
            m_maxVisibleChunks = 512;
            m_frustumCullingEnabled = true;
            m_backfaceCullingEnabled = true;
            m_chunkOptimizationEnabled = true;
            m_octreeCompressionEnabled = true;
            m_maxTextureSize = 512;
            break;
            
        case Preset::Medium:
            m_viewDistance = 8;
            m_maxVisibleChunks = 256;
            m_frustumCullingEnabled = true;
            m_backfaceCullingEnabled = true;
            m_chunkOptimizationEnabled = true;
            m_octreeCompressionEnabled = true;
            m_maxTextureSize = 256;
            break;
            
        case Preset::Low:
            m_viewDistance = 6;
            m_maxVisibleChunks = 128;
            m_frustumCullingEnabled = true;
            m_backfaceCullingEnabled = true;
            m_chunkOptimizationEnabled = true;
            m_octreeCompressionEnabled = true;
            m_maxTextureSize = 128;
            break;
            
        case Preset::Minimal:
            m_viewDistance = 4;
            m_maxVisibleChunks = 64;
            m_frustumCullingEnabled = true;
            m_backfaceCullingEnabled = true;
            m_chunkOptimizationEnabled = true;
            m_octreeCompressionEnabled = true;
            m_maxTextureSize = 64;
            break;
    }
    
    // Emit all signals
    emit viewDistanceChanged(m_viewDistance);
    emit maxVisibleChunksChanged(m_maxVisibleChunks);
    emit frustumCullingEnabledChanged(m_frustumCullingEnabled);
    emit backfaceCullingEnabledChanged(m_backfaceCullingEnabled);
    emit chunkOptimizationEnabledChanged(m_chunkOptimizationEnabled);
    emit octreeCompressionEnabledChanged(m_octreeCompressionEnabled);
    emit maxTextureSizeChanged(m_maxTextureSize);
    
    // Emit generic change signal
    emit settingsChanged();
    
    qDebug() << "Applied performance preset:" << static_cast<int>(preset);
}

int PerformanceSettings::getViewDistance() const {
    return m_viewDistance;
}

void PerformanceSettings::setViewDistance(int distance) {
    if (m_viewDistance != distance) {
        m_viewDistance = distance;
        emit viewDistanceChanged(distance);
        emit settingsChanged();
    }
}

int PerformanceSettings::getMaxVisibleChunks() const {
    return m_maxVisibleChunks;
}

void PerformanceSettings::setMaxVisibleChunks(int maxChunks) {
    if (m_maxVisibleChunks != maxChunks) {
        m_maxVisibleChunks = maxChunks;
        emit maxVisibleChunksChanged(maxChunks);
        emit settingsChanged();
    }
}

bool PerformanceSettings::isFrustumCullingEnabled() const {
    return m_frustumCullingEnabled;
}

void PerformanceSettings::setFrustumCullingEnabled(bool enabled) {
    if (m_frustumCullingEnabled != enabled) {
        m_frustumCullingEnabled = enabled;
        emit frustumCullingEnabledChanged(enabled);
        emit settingsChanged();
    }
}

bool PerformanceSettings::isBackfaceCullingEnabled() const {
    return m_backfaceCullingEnabled;
}

void PerformanceSettings::setBackfaceCullingEnabled(bool enabled) {
    if (m_backfaceCullingEnabled != enabled) {
        m_backfaceCullingEnabled = enabled;
        emit backfaceCullingEnabledChanged(enabled);
        emit settingsChanged();
    }
}

bool PerformanceSettings::isChunkOptimizationEnabled() const {
    return m_chunkOptimizationEnabled;
}

void PerformanceSettings::setChunkOptimizationEnabled(bool enabled) {
    if (m_chunkOptimizationEnabled != enabled) {
        m_chunkOptimizationEnabled = enabled;
        emit chunkOptimizationEnabledChanged(enabled);
        emit settingsChanged();
    }
}

bool PerformanceSettings::isOctreeCompressionEnabled() const {
    return m_octreeCompressionEnabled;
}

void PerformanceSettings::setOctreeCompressionEnabled(bool enabled) {
    if (m_octreeCompressionEnabled != enabled) {
        m_octreeCompressionEnabled = enabled;
        emit octreeCompressionEnabledChanged(enabled);
        emit settingsChanged();
    }
}

int PerformanceSettings::getMaxTextureSize() const {
    return m_maxTextureSize;
}

void PerformanceSettings::setMaxTextureSize(int size) {
    if (m_maxTextureSize != size) {
        m_maxTextureSize = size;
        emit maxTextureSizeChanged(size);
        emit settingsChanged();
    }
}

QMap<QString, QVariant> PerformanceSettings::getAllSettings() const {
    QMap<QString, QVariant> settings;
    settings["viewDistance"] = m_viewDistance;
    settings["maxVisibleChunks"] = m_maxVisibleChunks;
    settings["frustumCullingEnabled"] = m_frustumCullingEnabled;
    settings["backfaceCullingEnabled"] = m_backfaceCullingEnabled;
    settings["chunkOptimizationEnabled"] = m_chunkOptimizationEnabled;
    settings["octreeCompressionEnabled"] = m_octreeCompressionEnabled;
    settings["maxTextureSize"] = m_maxTextureSize;
    return settings;
}

void PerformanceSettings::setSetting(const QString& name, const QVariant& value) {
    if (name == "viewDistance") {
        setViewDistance(value.toInt());
    } else if (name == "maxVisibleChunks") {
        setMaxVisibleChunks(value.toInt());
    } else if (name == "frustumCullingEnabled") {
        setFrustumCullingEnabled(value.toBool());
    } else if (name == "backfaceCullingEnabled") {
        setBackfaceCullingEnabled(value.toBool());
    } else if (name == "chunkOptimizationEnabled") {
        setChunkOptimizationEnabled(value.toBool());
    } else if (name == "octreeCompressionEnabled") {
        setOctreeCompressionEnabled(value.toBool());
    } else if (name == "maxTextureSize") {
        setMaxTextureSize(value.toInt());
    } else {
        qWarning() << "Unknown performance setting:" << name;
    }
}