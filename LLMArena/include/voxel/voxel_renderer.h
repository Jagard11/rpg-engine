// include/voxel/voxel_renderer.h
#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include "voxel_world.h"
#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector>

// Class to render voxels using OpenGL
class VoxelRenderer : public QObject, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit VoxelRenderer(QObject* parent = nullptr);
    ~VoxelRenderer();
    
    // Initialize OpenGL resources
    void initialize();
    
    // Set world reference
    void setWorld(VoxelWorld* world);
    
    // Render the voxel world
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix);
    
public slots:
    // Update rendering data when world changes
    void updateRenderData();
    
private:
    VoxelWorld* m_world;
    
    // OpenGL resources
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram* m_shaderProgram;
    
    // Rendering data
    struct RenderVoxel {
        VoxelPos pos;
        QColor color;
    };
    
    QVector<RenderVoxel> m_visibleVoxels;
    int m_voxelCount;
    
    // Helper functions
    void createCubeGeometry(float size);
    void createShaders();
};

#endif // VOXEL_RENDERER_H