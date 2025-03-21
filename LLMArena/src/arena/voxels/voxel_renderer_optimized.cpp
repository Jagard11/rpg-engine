// src/arena/voxels/voxel_renderer_optimized.cpp
#include "../../include/arena/voxels/voxel_renderer.h"
#include "../../include/arena/voxels/culling/view_frustum.h"
#include <QOpenGLContext>
#include <QDebug>
#include <algorithm>

// Constructor - keep this simple to avoid memory allocation during init
VoxelRenderer::VoxelRenderer(QObject* parent) 
    : QObject(parent),
      m_world(nullptr), 
      m_vertexBuffer(QOpenGLBuffer::VertexBuffer),
      m_indexBuffer(QOpenGLBuffer::IndexBuffer),
      m_shaderProgram(nullptr),
      m_voxelCount(0),
      m_maxVisibleChunks(256),
      m_frustumCullingEnabled(true),
      m_backfaceCullingEnabled(true) {
    
    // Initialize view frustum
    m_viewFrustum = std::make_unique<ViewFrustum>();
    
    // Initialize texture map
    m_textures["cobblestone"] = nullptr;
    m_textures["grass"] = nullptr;
    m_textures["dirt"] = nullptr;
    m_textures["default"] = nullptr;
}

void VoxelRenderer::initialize() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Create shader program
    createShaders();
    
    // Create VAO
    m_vao.create();
    
    // Create buffers
    m_vertexBuffer.create();
    m_indexBuffer.create();
    
    // Create cube geometry (will be instanced for each voxel)
    createCubeGeometry(1.0f); // 1-meter cube
    
    // Load textures
    loadTextures();
}

void VoxelRenderer::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix) {
    if (!m_world || !m_shaderProgram) return;
    
    // Combine view and projection matrices (for frustum extraction)
    QMatrix4x4 viewProjection = projectionMatrix * viewMatrix;
    
    // Update view frustum for culling
    if (m_frustumCullingEnabled) {
        m_viewFrustum->update(viewProjection);
    }
    
    // Enable or disable backface culling
    if (m_backfaceCullingEnabled) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
    
    // Bind shader
    if (!m_shaderProgram->bind()) {
        qCritical() << "Failed to bind shader program";
        return;
    }
    
    // Set common uniforms
    m_shaderProgram->setUniformValue("view", viewMatrix);
    m_shaderProgram->setUniformValue("projection", projectionMatrix);
    
    // Extract camera position
    QMatrix4x4 invView = viewMatrix.inverted();
    QVector3D camPos = invView * QVector3D(0, 0, 0);
    m_shaderProgram->setUniformValue("viewPos", camPos);
    
    // Setup lighting (simplified for performance)
    QVector3D lightPos(0.0f, 1000.0f, 0.0f); // Light from above
    QVector3D lightColor(1.0f, 1.0f, 0.95f); // Slightly warm light
    m_shaderProgram->setUniformValue("lightPos", lightPos);
    m_shaderProgram->setUniformValue("lightColor", lightColor);
    m_shaderProgram->setUniformValue("ambientStrength", 0.3f);
    
    // Bind VAO
    m_vao.bind();
    
    // Enable texture unit 0
    glActiveTexture(GL_TEXTURE0);
    m_shaderProgram->setUniformValue("textureSampler", 0);
    
    // Track current bound texture to avoid redundant binds
    GLuint currentTexture = 0;
    
    // Keep count of drawn chunks and voxels for debugging
    int drawnChunks = 0;
    int drawnVoxels = 0;
    
    // First, organize voxels by chunk and material type for more efficient rendering
    struct ChunkBatch {
        ChunkCoordinate coord;
        std::vector<RenderVoxel> voxels;
        float distanceToCamera;
    };
    
    // Create a list of chunks to render
    std::vector<ChunkBatch> chunksToRender;
    
    // Group voxels by chunk
    std::unordered_map<ChunkCoordinate, ChunkBatch> chunkBatches;
    for (const RenderVoxel& voxel : m_visibleVoxels) {
        // Get chunk coordinate for this voxel
        ChunkCoordinate chunkCoord = ChunkCoordinate::fromWorldPosition(voxel.pos.toVector3D());
        
        // Skip chunks outside frustum
        if (m_frustumCullingEnabled && !m_viewFrustum->isChunkInside(chunkCoord)) {
            continue;
        }
        
        // Add to the appropriate chunk batch
        chunkBatches[chunkCoord].coord = chunkCoord;
        chunkBatches[chunkCoord].voxels.push_back(voxel);
        
        // Calculate distance to camera (once per chunk)
        if (chunkBatches[chunkCoord].voxels.size() == 1) {
            QVector3D chunkCenter = chunkCoord.getCenter();
            chunkBatches[chunkCoord].distanceToCamera = (chunkCenter - camPos).lengthSquared();
        }
    }
    
    // Convert map to vector for sorting
    for (auto& pair : chunkBatches) {
        chunksToRender.push_back(pair.second);
    }
    
    // Sort chunks by distance (front to back for better Z-culling)
    std::sort(chunksToRender.begin(), chunksToRender.end(), 
              [](const ChunkBatch& a, const ChunkBatch& b) {
                  return a.distanceToCamera < b.distanceToCamera;
              });
    
    // Limit the number of chunks to render if there are too many
    if (chunksToRender.size() > m_maxVisibleChunks) {
        chunksToRender.resize(m_maxVisibleChunks);
    }
    
    // Now render all chunks
    for (const ChunkBatch& batch : chunksToRender) {
        // Sort voxels by material type to minimize texture switches
        std::vector<RenderVoxel> sortedVoxels = batch.voxels;
        std::stable_sort(sortedVoxels.begin(), sortedVoxels.end(),
                  [](const RenderVoxel& a, const RenderVoxel& b) {
                      return static_cast<int>(a.type) < static_cast<int>(b.type);
                  });
        
        // Now render the voxels in this chunk
        VoxelType currentType = VoxelType::Air;
        
        for (const RenderVoxel& voxel : sortedVoxels) {
            // Set voxel-specific uniforms
            m_shaderProgram->setUniformValue("voxelPosition", voxel.pos.toVector3D());
            m_shaderProgram->setUniformValue("voxelColor", QVector4D(
                voxel.color.redF(), voxel.color.greenF(), voxel.color.blueF(), voxel.color.alphaF()));
            
            // Bind new texture only if voxel type changed
            if (voxel.type != currentType) {
                currentType = voxel.type;
                
                // Select texture based on voxel type
                QOpenGLTexture* texture = nullptr;
                bool useTexture = true;
                
                switch (voxel.type) {
                    case VoxelType::Cobblestone:
                        texture = m_textures["cobblestone"];
                        break;
                    case VoxelType::Grass:
                        texture = m_textures["grass"];
                        break;
                    case VoxelType::Dirt:
                        texture = m_textures["dirt"];
                        break;
                    case VoxelType::Solid:
                    default:
                        texture = m_textures["default"];
                        useTexture = false; // Use color only for generic solids
                        break;
                }
                
                // Bind texture if available and different from current
                if (texture && texture->isCreated()) {
                    // Only rebind if it's a different texture
                    if (texture->textureId() != currentTexture) {
                        texture->bind();
                        currentTexture = texture->textureId();
                    }
                    m_shaderProgram->setUniformValue("useTexture", useTexture);
                } else {
                    // If no valid texture, use default color
                    m_shaderProgram->setUniformValue("useTexture", false);
                }
            }
            
            // Draw cube
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
            drawnVoxels++;
        }
        
        drawnChunks++;
    }
    
    // Unbind any bound texture
    if (currentTexture != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    // Unbind VAO and shader
    m_vao.release();
    m_shaderProgram->release();
    
    // Disable culling when done
    if (m_backfaceCullingEnabled) {
        glDisable(GL_CULL_FACE);
    }
    
    // Debug output (every 60 frames to reduce spam)
    static int frameCounter = 0;
    if (frameCounter++ % 60 == 0) {
        qDebug() << "Rendering stats: Chunks:" << drawnChunks << "/" << chunkBatches.size()
                 << "Voxels:" << drawnVoxels << "/" << m_visibleVoxels.size();
    }
}

void VoxelRenderer::updateRenderData() {
    // Only update if we have a world
    if (!m_world) return;
    
    // Start with clean slate
    m_visibleVoxels.clear();
    
    // Get all loaded chunks
    QVector<VoxelPos> visiblePositions = m_world->getVisibleVoxels();
    
    // Convert to render voxels
    for (const VoxelPos& pos : visiblePositions) {
        Voxel voxel = m_world->getVoxel(pos);
        
        // Skip air voxels (invisible)
        if (voxel.type == VoxelType::Air) continue;
        
        RenderVoxel renderVoxel;
        renderVoxel.pos = pos;
        renderVoxel.color = voxel.color;
        renderVoxel.type = voxel.type;
        
        m_visibleVoxels.push_back(renderVoxel);
    }
    
    m_voxelCount = m_visibleVoxels.size();
    
    // Debug output
    qDebug() << "Updated render data: " << m_voxelCount << " visible voxels";
}

void VoxelRenderer::setMaxVisibleChunks(int maxChunks) {
    m_maxVisibleChunks = maxChunks;
}

void VoxelRenderer::setFrustumCullingEnabled(bool enabled) {
    m_frustumCullingEnabled = enabled;
}

void VoxelRenderer::setBackfaceCullingEnabled(bool enabled) {
    m_backfaceCullingEnabled = enabled;
}