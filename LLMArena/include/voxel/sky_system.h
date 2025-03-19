// include/voxel/sky_system.h
#ifndef SKY_SYSTEM_H
#define SKY_SYSTEM_H

#include <QObject>
#include <QColor>
#include <QVector3D>
#include <QDateTime>
#include <QTimer>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QPainter>

// Class to manage sky, sun, and moon
class SkySystem : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit SkySystem(QObject* parent = nullptr);
    ~SkySystem();
    
    // Initialize OpenGL resources
    void initialize();
    
    // Update sky, sun, and moon positions based on time
    void update();
    
    // Render the sky, sun, and moon
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix);
    
    // Get current sky color
    QColor getSkyColor() const;
    
    // Get sun and moon positions
    QVector3D getSunPosition() const;
    QVector3D getMoonPosition() const;
    
    // Set/get time update interval
    void setUpdateInterval(int msec);
    int updateInterval() const;
    
signals:
    void skyColorChanged(const QColor& color);
    void sunPositionChanged(const QVector3D& position);
    void moonPositionChanged(const QVector3D& position);
    
private slots:
    void updateTime();
    
private:
    // Time tracking
    QDateTime m_currentTime;
    QTimer m_updateTimer;
    
    // Sky properties
    QColor m_skyColor;
    float m_skyboxRadius;
    
    // Sun properties
    QVector3D m_sunPosition;
    float m_sunRadius;
    QOpenGLTexture* m_sunTexture;
    
    // Moon properties
    QVector3D m_moonPosition;
    float m_moonRadius;
    QOpenGLTexture* m_moonTexture;
    
    // OpenGL resources
    QOpenGLBuffer m_skyboxVBO;
    QOpenGLVertexArrayObject m_skyboxVAO;
    QOpenGLShaderProgram* m_skyboxShader;
    
    QOpenGLBuffer m_celestialVBO;
    QOpenGLVertexArrayObject m_celestialVAO;
    QOpenGLShaderProgram* m_celestialShader;
    
    // Helper functions
    void createSkyboxGeometry();
    void createCelestialGeometry();
    void createShaders();
    void updateCelestialPositions();
    void calculateSkyColor();
    
    // Local calculation methods that call the simpler versions
    QVector3D calculateSunPosition(const QDateTime& time);
    QVector3D calculateMoonPosition(const QDateTime& time);
};

#endif // SKY_SYSTEM_H