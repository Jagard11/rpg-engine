// src/voxel/sky_system.cpp
#include "../../include/voxel/sky_system.h"
#include <QDebug>
#include <QImage>
#include <cmath>
#include <stdexcept>

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

void SkySystem::createShaders() {
    // Create skybox shader program
    m_skyboxShader = new QOpenGLShaderProgram();
    
    // Skybox vertex shader
    const char* skyboxVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 texCoord;
        
        void main() {
            texCoord = position;
            vec4 pos = projection * view * vec4(position, 1.0);
            gl_Position = pos.xyww; // Ensure skybox is always at maximum depth
        }
    )";
    
    // Skybox fragment shader with gradient sky
    const char* skyboxFragmentShaderSource = R"(
        #version 330 core
        in vec3 texCoord;
        
        uniform vec3 skyColorZenith;
        uniform vec3 skyColorHorizon;
        
        out vec4 fragColor;
        
        void main() {
            // Calculate height above horizon (y component normalized)
            float height = (texCoord.y + 1.0) * 0.5;
            
            // Blend between horizon and zenith color
            vec3 skyColor = mix(skyColorHorizon, skyColorZenith, height);
            
            // Apply some atmospheric scattering effect
            float scattering = pow(1.0 - height, 2.0) * 0.2;
            skyColor = mix(skyColor, vec3(1.0, 1.0, 1.0), scattering);
            
            fragColor = vec4(skyColor, 1.0);
        }
    )";
    
    // Celestial (sun/moon) vertex shader
    const char* celestialVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 texCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec2 fragTexCoord;
        
        void main() {
            gl_Position = projection * view * model * vec4(position, 1.0);
            fragTexCoord = texCoord;
        }
    )";
    
    // Celestial fragment shader
    const char* celestialFragmentShaderSource = R"(
        #version 330 core
        in vec2 fragTexCoord;
        
        uniform sampler2D textureSampler;
        
        out vec4 fragColor;
        
        void main() {
            vec4 texColor = texture(textureSampler, fragTexCoord);
            fragColor = texColor;
        }
    )";
    
    // Compile and link skybox shader
    if (!m_skyboxShader->addShaderFromSourceCode(QOpenGLShader::Vertex, skyboxVertexShaderSource)) {
        qCritical() << "Failed to compile skybox vertex shader:" << m_skyboxShader->log();
    }
    
    if (!m_skyboxShader->addShaderFromSourceCode(QOpenGLShader::Fragment, skyboxFragmentShaderSource)) {
        qCritical() << "Failed to compile skybox fragment shader:" << m_skyboxShader->log();
    }
    
    if (!m_skyboxShader->link()) {
        qCritical() << "Failed to link skybox shader program:" << m_skyboxShader->log();
    }
    
    // Compile and link celestial shader
    m_celestialShader = new QOpenGLShaderProgram();
    
    if (!m_celestialShader->addShaderFromSourceCode(QOpenGLShader::Vertex, celestialVertexShaderSource)) {
        qCritical() << "Failed to compile celestial vertex shader:" << m_celestialShader->log();
    }
    
    if (!m_celestialShader->addShaderFromSourceCode(QOpenGLShader::Fragment, celestialFragmentShaderSource)) {
        qCritical() << "Failed to compile celestial fragment shader:" << m_celestialShader->log();
    }
    
    if (!m_celestialShader->link()) {
        qCritical() << "Failed to link celestial shader program:" << m_celestialShader->log();
    }
}

void SkySystem::createSkyboxGeometry() {
    // Use a much simpler approach - just create a simple quad for sky background
    // This will reduce potential problems with complex geometry
    
    float vertices[] = {
        // Position (screenspace quad)
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f
    };
    
    // Create and bind VAO
    m_skyboxVAO.create();
    m_skyboxVAO.bind();
    
    // Create and bind VBO
    m_skyboxVBO.create();
    m_skyboxVBO.bind();
    m_skyboxVBO.allocate(vertices, sizeof(vertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    
    // Unbind
    m_skyboxVAO.release();
    m_skyboxVBO.release();
}

void SkySystem::createCelestialGeometry() {
    // Create a quad for billboard rendering of sun and moon
    float celestialVertices[] = {
        // Positions          // Texture Coords
        -1.0f, -1.0f, 0.0f,   0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,   1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,   1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,   0.0f, 1.0f
    };
    
    // Create and bind VAO
    m_celestialVAO.create();
    m_celestialVAO.bind();
    
    // Create and bind VBO
    m_celestialVBO.create();
    m_celestialVBO.bind();
    m_celestialVBO.allocate(celestialVertices, sizeof(celestialVertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 
                         reinterpret_cast<void*>(3 * sizeof(float)));
    
    // Unbind
    m_celestialVAO.release();
    m_celestialVBO.release();
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
    // Calculate sun position
    m_sunPosition = calculateSunPosition(m_currentTime);
    
    // Calculate moon position
    m_moonPosition = calculateMoonPosition(m_currentTime);
    
    // Emit signals
    emit sunPositionChanged(m_sunPosition);
    emit moonPositionChanged(m_moonPosition);
}

QVector3D SkySystem::calculateSunPosition(const QDateTime& time) {
    // Simplified sun position calculation based on time of day
    // For more accurate calculations, you'd use astronomical algorithms
    
    // Extract hour and minute as decimal
    int hour = time.time().hour();
    int minute = time.time().minute();
    float timeOfDay = hour + minute / 60.0f;
    
    // Calculate angle (0 at midnight, π at noon)
    float angle = (timeOfDay / 24.0f) * 2.0f * M_PI;
    
    // Sun position on a circle
    float x = m_skyboxRadius * 0.8f * cos(angle);
    float y = m_skyboxRadius * 0.8f * sin(angle);
    float z = 0.0f;
    
    // Adjust for more realistic path
    if (y < 0) {
        y *= 0.3f; // Flatten when sun is below horizon
    }
    
    return QVector3D(x, y, z);
}

QVector3D SkySystem::calculateMoonPosition(const QDateTime& time) {
    // Moon is roughly opposite to sun
    QVector3D sunPos = calculateSunPosition(time);
    
    // Offset to avoid exact opposition
    float angleOffset = 0.2f * M_PI;
    
    // Calculate moon position
    float x = -sunPos.x() * 0.9f * cos(angleOffset);
    float y = -sunPos.y() * 0.9f * sin(angleOffset);
    float z = m_skyboxRadius * 0.1f * sin(angleOffset); // Slight z-offset
    
    return QVector3D(x, y, z);
}

void SkySystem::calculateSkyColor() {
    // Calculate sky color based on sun position
    float sunHeight = m_sunPosition.y() / m_skyboxRadius;
    
    // Day color (blue)
    QColor dayColor(90, 160, 255);
    
    // Sunset/sunrise color (orange)
    QColor sunsetColor(223, 127, 88);  // #df7f58
    
    // Night color (dark blue)
    QColor nightColor(10, 10, 50);
    
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
        // Silent exception handling
    }
}

void SkySystem::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    // Check if necessary components exist
    if (!m_skyboxShader || !m_celestialShader) {
        return;
    }
    
    // Check for valid VAOs
    if (!m_skyboxVAO.isCreated() || !m_celestialVAO.isCreated()) {
        return;
    }
    
    try {
        // Enable depth testing but disable depth writing for skybox
        glDepthMask(GL_FALSE);
        
        // Render skybox
        if (m_skyboxShader && m_skyboxShader->bind()) {
            // Use a simplified view matrix for sky (remove translation)
            QMatrix4x4 skyboxView = viewMatrix;
            skyboxView.setColumn(3, QVector4D(0, 0, 0, 1));
            
            // Set uniforms
            if (m_skyboxShader->uniformLocation("view") != -1)
                m_skyboxShader->setUniformValue("view", skyboxView);
            
            if (m_skyboxShader->uniformLocation("projection") != -1)
                m_skyboxShader->setUniformValue("projection", projectionMatrix);
            
            // Set sky colors
            QVector3D zenithColor(m_skyColor.redF(), m_skyColor.greenF(), m_skyColor.blueF());
            
            // Horizon color (slightly lighter)
            QVector3D horizonColor;
            if (m_sunPosition.y() > 0) {
                // Day: whiter horizon
                horizonColor = QVector3D(
                    qMin(1.0f, zenithColor.x() + 0.2f),
                    qMin(1.0f, zenithColor.y() + 0.2f),
                    qMin(1.0f, zenithColor.z() + 0.1f)
                );
            } else {
                // Night: slightly different color
                horizonColor = QVector3D(
                    qMin(1.0f, zenithColor.x() + 0.05f),
                    qMin(1.0f, zenithColor.y() + 0.05f),
                    qMin(1.0f, zenithColor.z() + 0.1f)
                );
            }
            
            if (m_skyboxShader->uniformLocation("skyColorZenith") != -1)
                m_skyboxShader->setUniformValue("skyColorZenith", zenithColor);
            
            if (m_skyboxShader->uniformLocation("skyColorHorizon") != -1)
                m_skyboxShader->setUniformValue("skyColorHorizon", horizonColor);
            
            // Draw skybox (just a quad now)
            m_skyboxVAO.bind();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            m_skyboxVAO.release();
            
            m_skyboxShader->release();
        }
        
        // Re-enable depth writing
        glDepthMask(GL_TRUE);
        
        // Don't render sun and moon for now - simplify to reduce potential issues
        
    } catch (...) {
        // Silent exception handling
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