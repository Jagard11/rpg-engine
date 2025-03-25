// include/arena/voxels/voxel_renderer.h
#ifndef VOXEL_RENDERER_H
#define VOXEL_RENDERER_H

#include "voxel_world.h"
#include "../system/performance_settings.h"
#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector>
#include <QMap>
#include <memory>

// Forward declarations - full definition will be included in the cpp file
class ViewFrustum;
class PlayerEntity;

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
    
    // Set player entity reference
    void setPlayerEntity(PlayerEntity* playerEntity);
    
    // Render the voxel world
    void render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix);
    
    // Render the voxel world using player entity for culling
    void renderWithPlayer(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix, PlayerEntity* playerEntity);
    
    // Performance settings
    void setMaxVisibleChunks(int maxChunks);
    void setFrustumCullingEnabled(bool enabled);
    void setBackfaceCullingEnabled(bool enabled);
    
    // Current settings
    int getMaxVisibleChunks() const { return m_maxVisibleChunks; }
    bool isFrustumCullingEnabled() const { return m_frustumCullingEnabled; }
    bool isBackfaceCullingEnabled() const { return m_backfaceCullingEnabled; }
    
public slots:
    // Update rendering data when world changes
    void updateRenderData();
    
    // Update settings from performance settings
    void updateSettings();
    
private:
    VoxelWorld* m_world;
    PlayerEntity* m_playerEntity;
    PerformanceSettings* m_perfSettings;
    
    // Performance settings
    int m_maxVisibleChunks;
    bool m_frustumCullingEnabled;
    bool m_backfaceCullingEnabled;
    
    // View frustum for culling - using pimpl idiom to avoid incomplete type issues
    std::unique_ptr<ViewFrustum> m_viewFrustum;
    
    // OpenGL resources
    QOpenGLBuffer m_vertexBuffer;
    QOpenGLBuffer m_indexBuffer;
    QOpenGLVertexArrayObject m_vao;
    QOpenGLShaderProgram* m_shaderProgram;
    
    // Texture resources
    QMap<QString, QOpenGLTexture*> m_textures;
    
    // Rendering data
    struct RenderVoxel {
        VoxelPos pos;
        QColor color;
        VoxelType type;
        bool isBoundary = false;  // Indicates voxel is at a chunk boundary
    };
    
    QVector<RenderVoxel> m_visibleVoxels;
    int m_voxelCount;
    
    // Helper functions
    void createCubeGeometry(float size);
    void createShaders();
    void loadTextures();
    void createTexture(const QString& name, const QImage& image);
    void createDefaultTexture(const QString& name);
};

#endif // VOXEL_RENDERER_H