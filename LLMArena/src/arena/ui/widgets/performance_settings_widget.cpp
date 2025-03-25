// src/arena/ui/widgets/performance_settings_widget.cpp
#include "../../../../include/arena/ui/widgets/performance_settings_widget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QApplication>

PerformanceSettingsWidget::PerformanceSettingsWidget(QWidget* parent)
    : QWidget(parent), m_frameCount(0) {
    
    // Get settings instance
    m_settings = PerformanceSettings::getInstance();
    
    // Setup UI
    setupUi();
    
    // Connect signals
    connectSignals();
    
    // Setup FPS counter
    setupFpsCounter();
    
    // Update UI from current settings
    updateUiFromSettings();
    
    // Hide by default
    hide();
    
    // Set window flags
    setWindowFlags(Qt::Window | Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_ShowWithoutActivating);
}

void PerformanceSettingsWidget::setupUi() {
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);
    
    // Title and close button
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("Performance Settings");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    QPushButton* closeButton = new QPushButton("X");
    closeButton->setMaximumWidth(30);
    connect(closeButton, &QPushButton::clicked, this, &PerformanceSettingsWidget::hide);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);
    
    mainLayout->addLayout(titleLayout);
    
    // Presets group
    QGroupBox* presetGroup = new QGroupBox("Presets");
    QHBoxLayout* presetLayout = new QHBoxLayout(presetGroup);
    
    m_presetComboBox = new QComboBox();
    m_presetComboBox->addItem("Ultra", static_cast<int>(PerformanceSettings::Preset::Ultra));
    m_presetComboBox->addItem("High", static_cast<int>(PerformanceSettings::Preset::High));
    m_presetComboBox->addItem("Medium", static_cast<int>(PerformanceSettings::Preset::Medium));
    m_presetComboBox->addItem("Low", static_cast<int>(PerformanceSettings::Preset::Low));
    m_presetComboBox->addItem("Minimal", static_cast<int>(PerformanceSettings::Preset::Minimal));
    
    // Select Minimal by default (best for ThinkPad)
    m_presetComboBox->setCurrentIndex(4);
    
    m_applyPresetButton = new QPushButton("Apply");
    
    presetLayout->addWidget(m_presetComboBox);
    presetLayout->addWidget(m_applyPresetButton);
    
    mainLayout->addWidget(presetGroup);
    
    // View distance
    QGroupBox* viewDistanceGroup = new QGroupBox("View Distance");
    QVBoxLayout* viewDistanceLayout = new QVBoxLayout(viewDistanceGroup);
    
    m_viewDistanceSlider = new QSlider(Qt::Horizontal);
    m_viewDistanceSlider->setMinimum(1);
    m_viewDistanceSlider->setMaximum(16);
    m_viewDistanceSlider->setTickInterval(1);
    m_viewDistanceSlider->setTickPosition(QSlider::TicksBelow);
    
    m_viewDistanceLabel = new QLabel("View Distance: 4 chunks");
    
    viewDistanceLayout->addWidget(m_viewDistanceLabel);
    viewDistanceLayout->addWidget(m_viewDistanceSlider);
    
    mainLayout->addWidget(viewDistanceGroup);
    
    // Max visible chunks
    QGroupBox* chunksGroup = new QGroupBox("Max Visible Chunks");
    QVBoxLayout* chunksLayout = new QVBoxLayout(chunksGroup);
    
    m_maxVisibleChunksSlider = new QSlider(Qt::Horizontal);
    m_maxVisibleChunksSlider->setMinimum(16);
    m_maxVisibleChunksSlider->setMaximum(1024);
    m_maxVisibleChunksSlider->setTickInterval(64);
    m_maxVisibleChunksSlider->setTickPosition(QSlider::TicksBelow);
    
    m_maxVisibleChunksLabel = new QLabel("Max Visible Chunks: 64");
    
    chunksLayout->addWidget(m_maxVisibleChunksLabel);
    chunksLayout->addWidget(m_maxVisibleChunksSlider);
    
    mainLayout->addWidget(chunksGroup);
    
    // Culling options
    QGroupBox* cullingGroup = new QGroupBox("Culling Options");
    QVBoxLayout* cullingLayout = new QVBoxLayout(cullingGroup);
    
    QLabel* cullingInfoLabel = new QLabel("Toggle which voxels are rendered to improve performance:");
    cullingInfoLabel->setWordWrap(true);
    cullingLayout->addWidget(cullingInfoLabel);
    
    m_frustumCullingCheckBox = new QCheckBox("Frustum Culling");
    m_frustumCullingCheckBox->setToolTip("Only render voxels that are within the camera's field of view");
    
    m_backfaceCullingCheckBox = new QCheckBox("Backface Culling");
    m_backfaceCullingCheckBox->setToolTip("Don't render faces of voxels that are facing away from the camera");
    
    m_occlusionCullingCheckBox = new QCheckBox("Occlusion Culling");
    m_occlusionCullingCheckBox->setToolTip("Don't render voxels that are completely surrounded by other voxels");
    
    cullingLayout->addWidget(m_frustumCullingCheckBox);
    cullingLayout->addWidget(m_backfaceCullingCheckBox);
    cullingLayout->addWidget(m_occlusionCullingCheckBox);
    
    mainLayout->addWidget(cullingGroup);
    
    // Optimization options
    QGroupBox* optimizationGroup = new QGroupBox("Optimization Options");
    QVBoxLayout* optimizationLayout = new QVBoxLayout(optimizationGroup);
    
    m_chunkOptimizationCheckBox = new QCheckBox("Chunk Optimization");
    m_octreeCompressionCheckBox = new QCheckBox("Octree Compression");
    
    optimizationLayout->addWidget(m_chunkOptimizationCheckBox);
    optimizationLayout->addWidget(m_octreeCompressionCheckBox);
    
    mainLayout->addWidget(optimizationGroup);
    
    // Texture settings
    QGroupBox* textureGroup = new QGroupBox("Texture Settings");
    QVBoxLayout* textureLayout = new QVBoxLayout(textureGroup);
    
    m_maxTextureSizeSlider = new QSlider(Qt::Horizontal);
    m_maxTextureSizeSlider->setMinimum(16);
    m_maxTextureSizeSlider->setMaximum(1024);
    m_maxTextureSizeSlider->setTickInterval(64);
    m_maxTextureSizeSlider->setTickPosition(QSlider::TicksBelow);
    
    m_maxTextureSizeLabel = new QLabel("Max Texture Size: 64");
    
    textureLayout->addWidget(m_maxTextureSizeLabel);
    textureLayout->addWidget(m_maxTextureSizeSlider);
    
    mainLayout->addWidget(textureGroup);
    
    // FPS counter
    QHBoxLayout* fpsLayout = new QHBoxLayout();
    m_fpsLabel = new QLabel("FPS: 0");
    m_fpsLabel->setStyleSheet("font-weight: bold;");
    fpsLayout->addWidget(m_fpsLabel);
    fpsLayout->addStretch();
    
    mainLayout->addLayout(fpsLayout);
    
    // Set size policy
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMinimumWidth(300);
}

void PerformanceSettingsWidget::connectSignals() {
    // Connect preset button
    connect(m_applyPresetButton, &QPushButton::clicked, this, &PerformanceSettingsWidget::onApplyPreset);
    
    // Connect sliders
    connect(m_viewDistanceSlider, &QSlider::valueChanged, this, &PerformanceSettingsWidget::onViewDistanceChanged);
    connect(m_maxVisibleChunksSlider, &QSlider::valueChanged, this, &PerformanceSettingsWidget::onMaxVisibleChunksChanged);
    connect(m_maxTextureSizeSlider, &QSlider::valueChanged, this, &PerformanceSettingsWidget::onMaxTextureSizeChanged);
    
    // Connect checkboxes
    connect(m_frustumCullingCheckBox, &QCheckBox::toggled, this, &PerformanceSettingsWidget::onFrustumCullingEnabledChanged);
    connect(m_backfaceCullingCheckBox, &QCheckBox::toggled, this, &PerformanceSettingsWidget::onBackfaceCullingEnabledChanged);
    connect(m_occlusionCullingCheckBox, &QCheckBox::toggled, this, &PerformanceSettingsWidget::onOcclusionCullingEnabledChanged);
    connect(m_chunkOptimizationCheckBox, &QCheckBox::toggled, this, &PerformanceSettingsWidget::onChunkOptimizationEnabledChanged);
    connect(m_octreeCompressionCheckBox, &QCheckBox::toggled, this, &PerformanceSettingsWidget::onOctreeCompressionEnabledChanged);
    
    // Connect settings changes back to UI updates
    connect(m_settings, &PerformanceSettings::settingsChanged, this, &PerformanceSettingsWidget::updateUiFromSettings);
}

void PerformanceSettingsWidget::setupFpsCounter() {
    // Start elapsed timer
    m_elapsedTimer.start();
    
    // Setup FPS update timer
    m_fpsTimer.setInterval(1000);  // Update every second
    connect(&m_fpsTimer, &QTimer::timeout, this, &PerformanceSettingsWidget::updateFpsCounter);
    m_fpsTimer.start();
}

void PerformanceSettingsWidget::toggleVisibility() {
    if (isVisible()) {
        hide();
    } else {
        show();
    }
}

void PerformanceSettingsWidget::onApplyPreset() {
    // Get selected preset
    PerformanceSettings::Preset preset = static_cast<PerformanceSettings::Preset>(
        m_presetComboBox->currentData().toInt()
    );
    
    // Log preset being applied
    QString presetName;
    switch (preset) {
        case PerformanceSettings::Preset::Ultra: presetName = "Ultra"; break;
        case PerformanceSettings::Preset::High: presetName = "High"; break;
        case PerformanceSettings::Preset::Medium: presetName = "Medium"; break;
        case PerformanceSettings::Preset::Low: presetName = "Low"; break;
        case PerformanceSettings::Preset::Minimal: presetName = "Minimal"; break;
        default: presetName = "Unknown"; break;
    }
    
    qDebug() << "PerformanceSettingsWidget: Applying preset" << presetName;
    
    // Apply preset
    m_settings->applyPreset(preset);
    
    // Log the settings after preset is applied
    qDebug() << "PerformanceSettingsWidget: Settings after applying preset:";
    qDebug() << "  - Occlusion Culling:" << (m_settings->isOcclusionCullingEnabled() ? "Enabled" : "Disabled");
    qDebug() << "  - Frustum Culling:" << (m_settings->isFrustumCullingEnabled() ? "Enabled" : "Disabled");
    qDebug() << "  - Backface Culling:" << (m_settings->isBackfaceCullingEnabled() ? "Enabled" : "Disabled");
}

void PerformanceSettingsWidget::onViewDistanceChanged(int value) {
    // Update label
    m_viewDistanceLabel->setText(QString("View Distance: %1 chunks").arg(value));
    
    // Update setting
    m_settings->setViewDistance(value);
}

void PerformanceSettingsWidget::onMaxVisibleChunksChanged(int value) {
    // Update label
    m_maxVisibleChunksLabel->setText(QString("Max Visible Chunks: %1").arg(value));
    
    // Update setting
    m_settings->setMaxVisibleChunks(value);
}

void PerformanceSettingsWidget::onFrustumCullingEnabledChanged(bool enabled) {
    m_settings->setFrustumCullingEnabled(enabled);
}

void PerformanceSettingsWidget::onBackfaceCullingEnabledChanged(bool enabled) {
    m_settings->setBackfaceCullingEnabled(enabled);
}

void PerformanceSettingsWidget::onOcclusionCullingEnabledChanged(bool enabled) {
    qDebug() << "PerformanceSettingsWidget: Occlusion culling toggled to" << (enabled ? "enabled" : "disabled");
    m_settings->setOcclusionCullingEnabled(enabled);
}

void PerformanceSettingsWidget::onChunkOptimizationEnabledChanged(bool enabled) {
    m_settings->setChunkOptimizationEnabled(enabled);
}

void PerformanceSettingsWidget::onOctreeCompressionEnabledChanged(bool enabled) {
    m_settings->setOctreeCompressionEnabled(enabled);
}

void PerformanceSettingsWidget::onMaxTextureSizeChanged(int value) {
    // Update label
    m_maxTextureSizeLabel->setText(QString("Max Texture Size: %1").arg(value));
    
    // Update setting
    m_settings->setMaxTextureSize(value);
}

void PerformanceSettingsWidget::updateFpsCounter() {
    // Calculate FPS
    qint64 elapsed = m_elapsedTimer.elapsed();
    if (elapsed > 0) {
        double fps = static_cast<double>(m_frameCount) * 1000.0 / elapsed;
        m_fpsLabel->setText(QString("FPS: %1").arg(fps, 0, 'f', 1));
    }
    
    // Reset counter
    m_frameCount = 0;
    m_elapsedTimer.restart();
}

void PerformanceSettingsWidget::updateUiFromSettings() {
    // Update sliders
    m_viewDistanceSlider->setValue(m_settings->getViewDistance());
    m_maxVisibleChunksSlider->setValue(m_settings->getMaxVisibleChunks());
    m_maxTextureSizeSlider->setValue(m_settings->getMaxTextureSize());
    
    // Update checkboxes
    m_frustumCullingCheckBox->setChecked(m_settings->isFrustumCullingEnabled());
    m_backfaceCullingCheckBox->setChecked(m_settings->isBackfaceCullingEnabled());
    m_occlusionCullingCheckBox->setChecked(m_settings->isOcclusionCullingEnabled());
    m_chunkOptimizationCheckBox->setChecked(m_settings->isChunkOptimizationEnabled());
    m_octreeCompressionCheckBox->setChecked(m_settings->isOctreeCompressionEnabled());
    
    // Update labels
    m_viewDistanceLabel->setText(QString("View Distance: %1 chunks").arg(m_settings->getViewDistance()));
    m_maxVisibleChunksLabel->setText(QString("Max Visible Chunks: %1").arg(m_settings->getMaxVisibleChunks()));
    m_maxTextureSizeLabel->setText(QString("Max Texture Size: %1").arg(m_settings->getMaxTextureSize()));
    
    // Add debug output to verify settings
    qDebug() << "PerformanceSettingsWidget: UI updated from settings:";
    qDebug() << "  - View Distance:" << m_settings->getViewDistance();
    qDebug() << "  - Max Visible Chunks:" << m_settings->getMaxVisibleChunks();
    qDebug() << "  - Frustum Culling:" << m_settings->isFrustumCullingEnabled();
    qDebug() << "  - Backface Culling:" << m_settings->isBackfaceCullingEnabled();
    qDebug() << "  - Occlusion Culling:" << m_settings->isOcclusionCullingEnabled();
    qDebug() << "  - Chunk Optimization:" << m_settings->isChunkOptimizationEnabled();
    qDebug() << "  - Octree Compression:" << m_settings->isOctreeCompressionEnabled();
    qDebug() << "  - Max Texture Size:" << m_settings->getMaxTextureSize();
}