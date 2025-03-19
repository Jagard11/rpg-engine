// src/voxel/sky_system.cpp
#include "../../include/voxel/sky_system.h"
#include "../../include/voxel/sky_system_helpers.h"
#include <QDebug>
#include <QImage>
#include <cmath>
#include <stdexcept>

// Forward declaration of astronomical calculation functions
QVector3D calculateSunPositionSimple(float skyboxRadius, const QDateTime& time);
QVector3D calculateMoonPositionSimple(float skyboxRadius, const QDateTime& time);
QVector3D calculateSunPositionAstronomical(float skyboxRadius, const QDateTime& time);
QVector3D calculateMoonPositionAstronomical(float skyboxRadius, const QDateTime& time);

SkySystem::SkySystem(QObject* parent) 
    : QObject(parent),
      m_currentTime(QDateTime::currentDateTime()),
      m_skyColor(Qt::blue),
      m_skyboxRadius(100.0f),
      m_sunPosition(0.0f, 50.0f, 0.0f),
      m_sunRadius(5.0f),
      m_sunTexture(nullptr),
      m_moonPosition(0.0f, -50.0f, 0.0f),
      m_moonRadius(3.0f),
      m_moonTexture(nullptr),
      m_skyboxVBO(QOpenGLBuffer::VertexBuffer),
      m_celestialVBO(QOpenGLBuffer::VertexBuffer),
      m_skyboxShader(nullptr),
      m_celestialShader(nullptr) {
    
    // Set up timer for time-based updates (lower frequency to reduce potential issues)
    m_updateTimer.setInterval(5000); // Update every 5 seconds
    connect(&m_updateTimer, &QTimer::timeout, this, &SkySystem::updateTime);
    
    // We'll start the timer only after initialization
}

SkySystem::~SkySystem() {
    // Only clean up if we have a valid OpenGL context
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context || !context->isValid()) {
        return;
    }
    
    // Clean up OpenGL resources
    if (m_skyboxVBO.isCreated()) {
        m_skyboxVBO.destroy();
    }
    
    if (m_skyboxVAO.isCreated()) {
        m_skyboxVAO.destroy();
    }
    
    if (m_celestialVBO.isCreated()) {
        m_celestialVBO.destroy();
    }
    
    if (m_celestialVAO.isCreated()) {
        m_celestialVAO.destroy();
    }
    
    delete m_skyboxShader;
    
    delete m_celestialShader;
    
    if (m_sunTexture) {
        if (m_sunTexture->isCreated()) {
            m_sunTexture->destroy();
        }
        delete m_sunTexture;
    }
    
    if (m_moonTexture) {
        if (m_moonTexture->isCreated()) {
            m_moonTexture->destroy();
        }
        delete m_moonTexture;
    }
}

void SkySystem::initialize() {
    try {
        // Initialize OpenGL functions
        initializeOpenGLFunctions();
        
        // Create shader programs
        createShaders();
        
        // Create skybox geometry
        createSkyboxGeometry();
        
        // Create sun and moon geometry
        createCelestialGeometry();
        
        // Create sun texture
        QImage sunImage(256, 256, QImage::Format_RGBA8888);
        sunImage.fill(Qt::transparent);
        
        {
            QPainter painter(&sunImage);
            painter.setRenderHint(QPainter::Antialiasing, true);
            
            // Draw sun with gradient
            QRadialGradient gradient(128, 128, 128);
            gradient.setColorAt(0, QColor(255, 255, 200));
            gradient.setColorAt(0.8, QColor(255, 200, 0));
            gradient.setColorAt(1, QColor(255, 100, 0, 0));
            
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(0, 0, 256, 256);
        }
        
        m_sunTexture = new QOpenGLTexture(sunImage);
        m_sunTexture->setMinificationFilter(QOpenGLTexture::Linear);
        m_sunTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        
        // Create moon texture
        QImage moonImage(256, 256, QImage::Format_RGBA8888);
        moonImage.fill(Qt::transparent);
        
        {
            QPainter painter(&moonImage);
            painter.setRenderHint(QPainter::Antialiasing, true);
            
            // Draw moon with gradient
            QRadialGradient gradient(128, 128, 128);
            gradient.setColorAt(0, QColor(230, 230, 230));
            gradient.setColorAt(0.8, QColor(200, 200, 210));
            gradient.setColorAt(1, QColor(180, 180, 210, 0));
            
            painter.setBrush(gradient);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(0, 0, 256, 256);
            
            // Add some craters
            painter.setBrush(QColor(180, 180, 180, 100));
            painter.drawEllipse(80, 60, 40, 40);
            painter.drawEllipse(160, 100, 30, 30);
            painter.drawEllipse(70, 140, 50, 50);
        }
        
        m_moonTexture = new QOpenGLTexture(moonImage);
        m_moonTexture->setMinificationFilter(QOpenGLTexture::Linear);
        m_moonTexture->setMagnificationFilter(QOpenGLTexture::Linear);
        
        // Update positions based on current time
        updateCelestialPositions();
        
        // Only now start the timer
        m_updateTimer.start();
    } catch (const std::exception& e) {
        qCritical() << "SkySystem initialization failed:" << e.what();
    } catch (...) {
        qCritical() << "SkySystem initialization failed with unknown error";
    }
}

void SkySystem::updateTime() {
    try {
        // Update current time
        m_currentTime = QDateTime::currentDateTime();
        
        // Update celestial positions
        updateCelestialPositions();
        
        // Update sky color
        calculateSkyColor();
    } catch (...) {
        // Silent exception handling to prevent crashes
    }
}

void SkySystem::updateCelestialPositions() {
    // Call the external functions with our parameters
    m_sunPosition = ::calculateSunPositionAstronomical(m_skyboxRadius, m_currentTime);
    m_moonPosition = ::calculateMoonPositionAstronomical(m_skyboxRadius, m_currentTime);
    
    // Emit signals
    emit sunPositionChanged(m_sunPosition);
    emit moonPositionChanged(m_moonPosition);
}

QVector3D SkySystem::calculateSunPosition(const QDateTime& time) {
    return ::calculateSunPositionSimple(m_skyboxRadius, time);
}

QVector3D SkySystem::calculateMoonPosition(const QDateTime& time) {
    return ::calculateMoonPositionSimple(m_skyboxRadius, time);
}

QVector3D SkySystem::calculateSunPositionAstronomical(const QDateTime& time) {
    return ::calculateSunPositionAstronomical(m_skyboxRadius, time);
}

QVector3D SkySystem::calculateMoonPositionAstronomical(const QDateTime& time) {
    return ::calculateMoonPositionAstronomical(m_skyboxRadius, time);
}

void SkySystem::calculateSkyColor() {
    // Calculate sky color based on sun position
    float sunHeight = m_sunPosition.y() / m_skyboxRadius;
    
    // Use a more consistent color palette
    // Day color (blue)
    QColor dayColor(100, 150, 255);
    
    // Sunset/sunrise color (orange)
    QColor sunsetColor(255, 130, 60);
    
    // Night color (dark blue)
    QColor nightColor(15, 20, 60);
    
    QColor newColor;
    
    if (sunHeight > 0.2f) {
        // Day
        newColor = dayColor;
    }
    else if (sunHeight > -0.2f) {
        // Sunrise/sunset: blend between day and sunset colors
        float t = (sunHeight + 0.2f) / 0.4f;
        newColor = QColor(
            dayColor.red() * t + sunsetColor.red() * (1.0f - t),
            dayColor.green() * t + sunsetColor.green() * (1.0f - t),
            dayColor.blue() * t + sunsetColor.blue() * (1.0f - t)
        );
    }
    else {
        // Night: blend between sunset and night colors
        float t = qMin(1.0f, (-sunHeight - 0.2f) / 0.4f);
        newColor = QColor(
            sunsetColor.red() * (1.0f - t) + nightColor.red() * t,
            sunsetColor.green() * (1.0f - t) + nightColor.green() * t,
            sunsetColor.blue() * (1.0f - t) + nightColor.blue() * t
        );
    }
    
    // Update color if changed
    if (m_skyColor != newColor) {
        m_skyColor = newColor;
        emit skyColorChanged(m_skyColor);
    }
}

void SkySystem::update() {
    try {
        // Update the time
        m_currentTime = QDateTime::currentDateTime();
        
        // Update positions and colors
        updateCelestialPositions();
        calculateSkyColor();
    } catch (...) {
        // Silent catch - just continue
    }
}

QColor SkySystem::getSkyColor() const {
    return m_skyColor;
}

QVector3D SkySystem::getSunPosition() const {
    return m_sunPosition;
}

QVector3D SkySystem::getMoonPosition() const {
    return m_moonPosition;
}

void SkySystem::setUpdateInterval(int msec) {
    m_updateTimer.setInterval(msec);
}

int SkySystem::updateInterval() const {
    return m_updateTimer.interval();
}