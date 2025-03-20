// include/arena/ui/voxel_highlight_renderer.h
#ifndef VOXEL_HIGHLIGHT_RENDERER_H
#define VOXEL_HIGHLIGHT_RENDERER_H

#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QVector3D>
#include <QVector>
#include <QMatrix4x4>

// Class for rendering voxel highlight when placing/removing voxels
class VoxelHighlightRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit VoxelHighlightRenderer(QObject* parent = nullptr);
    ~VoxelHighlightRenderer();
    
    // Initialize OpenGL resources
    void initialize();
    
    // Render voxel highlight at position with optional face highlight
    // face: 0-5 for specific face, -1 for all faces
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix, 
               const QVector3D& position, int highlightFace = -1);
    
private:
    // Create shader program
    void createShaders();
    
    // Create wireframe cube geometry
    void createWireframeCubeGeometry();
    
    // Create face highlight geometry
    void createFaceHighlightGeometry();
    
    // OpenGL resources
    QOpenGLShaderProgram* m_shader;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    QOpenGLBuffer m_ibo;
    int m_wireframeIndexCount;
    
    // Face highlight resources
    QVector<QOpenGLVertexArrayObject*> m_faceVAOs;
    QVector<QOpenGLBuffer*> m_faceVBOs;
    
    // Currently highlighted face (-1 if none)
    int m_highlightFace;
};

#endif // VOXEL_HIGHLIGHT_RENDERER_H