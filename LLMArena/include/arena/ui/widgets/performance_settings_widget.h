// include/arena/ui/widgets/performance_settings_widget.h
#ifndef PERFORMANCE_SETTINGS_WIDGET_H
#define PERFORMANCE_SETTINGS_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>
#include <QElapsedTimer>

#include "../../system/performance_settings.h"

/**
 * @brief UI widget for adjusting performance settings.
 * 
 * This widget provides sliders, checkboxes, and preset options
 * for adjusting the performance settings in real-time.
 */
class PerformanceSettingsWidget : public QWidget {
    Q_OBJECT
    
public:
    /**
     * @brief Construct a performance settings widget
     * @param parent Parent widget
     */
    explicit PerformanceSettingsWidget(QWidget* parent = nullptr);
    
    /**
     * @brief Show the widget if hidden, hide if shown
     */
    void toggleVisibility();
    
    /**
     * @brief Update the UI from current settings
     */
    void updateUiFromSettings();
    
private slots:
    /**
     * @brief Apply a preset from the preset combo box
     */
    void onApplyPreset();
    
    /**
     * @brief Update view distance from slider
     */
    void onViewDistanceChanged(int value);
    
    /**
     * @brief Update max visible chunks from slider
     */
    void onMaxVisibleChunksChanged(int value);
    
    /**
     * @brief Update frustum culling enabled from checkbox
     */
    void onFrustumCullingEnabledChanged(bool enabled);
    
    /**
     * @brief Update backface culling enabled from checkbox
     */
    void onBackfaceCullingEnabledChanged(bool enabled);
    
    /**
     * @brief Update occlusion culling enabled from checkbox
     */
    void onOcclusionCullingEnabledChanged(bool enabled);
    
    /**
     * @brief Update chunk optimization enabled from checkbox
     */
    void onChunkOptimizationEnabledChanged(bool enabled);
    
    /**
     * @brief Update octree compression enabled from checkbox
     */
    void onOctreeCompressionEnabledChanged(bool enabled);
    
    /**
     * @brief Update max texture size from slider
     */
    void onMaxTextureSizeChanged(int value);
    
    /**
     * @brief Update FPS counter
     */
    void updateFpsCounter();
    
private:
    // Performance settings reference
    PerformanceSettings* m_settings;
    
    // Presets
    QComboBox* m_presetComboBox;
    QPushButton* m_applyPresetButton;
    
    // View distance
    QSlider* m_viewDistanceSlider;
    QLabel* m_viewDistanceLabel;
    
    // Max visible chunks
    QSlider* m_maxVisibleChunksSlider;
    QLabel* m_maxVisibleChunksLabel;
    
    // Culling options
    QCheckBox* m_frustumCullingCheckBox;
    QCheckBox* m_backfaceCullingCheckBox;
    QCheckBox* m_occlusionCullingCheckBox;
    
    // Optimization options
    QCheckBox* m_chunkOptimizationCheckBox;
    QCheckBox* m_octreeCompressionCheckBox;
    
    // Texture settings
    QSlider* m_maxTextureSizeSlider;
    QLabel* m_maxTextureSizeLabel;
    
    // FPS counter
    QLabel* m_fpsLabel;
    QTimer m_fpsTimer;
    int m_frameCount;
    QElapsedTimer m_elapsedTimer;
    
    /**
     * @brief Setup the UI
     */
    void setupUi();
    
    /**
     * @brief Connect signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Register for frame counter updates
     */
    void setupFpsCounter();
};

#endif // PERFORMANCE_SETTINGS_WIDGET_H