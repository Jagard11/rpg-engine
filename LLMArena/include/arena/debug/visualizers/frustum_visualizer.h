// include/arena/debug/visualizers/frustum_visualizer.h
#ifndef FRUSTUM_VISUALIZER_H
#define FRUSTUM_VISUALIZER_H

#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <memory>
#include <QVector3D>
#include <QVector>

// Forward declarations
class ViewFrustum;

/**
 * @brief Visualizer for the view frustum culling box
 * 
 * This class renders a visual representation of the frustum culling box
 * to help developers understand how frustum culling works.
 */
class FrustumVisualizer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit FrustumVisualizer(QObject* parent = nullptr);
    ~FrustumVisualizer();
    
    /**
     * @brief Initialize OpenGL resources
     */
    void initialize();
    
    /**
     * @brief Render the frustum visualization
     * @param viewMatrix Camera view matrix
     * @param projectionMatrix Camera projection matrix
     */
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix);
    
    /**
     * @brief Set whether the visualizer is enabled
     * @param enabled Enabled flag
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if the visualizer is enabled
     * @return True if enabled
     */
    bool isEnabled() const;
    
private:
    // Visualization state
    bool m_enabled;
    
    // OpenGL resources
    QOpenGLShaderProgram* m_shader;
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
    
    // Frustum data
    QVector<QVector3D> m_frustumPoints;
    std::unique_ptr<ViewFrustum> m_viewFrustum;
    
    // Number of indices for drawing
    int m_wireframeIndexCount;
    
    // Create shader program
    void createShaders();
    
    // Create frustum geometry
    void createFrustumGeometry();
    
    // Update frustum geometry from matrices
    void updateFrustumGeometry(const QMatrix4x4& viewProjectionInverse);
};

#endif // FRUSTUM_VISUALIZER_H