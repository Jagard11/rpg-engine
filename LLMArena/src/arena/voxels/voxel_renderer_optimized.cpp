// src/arena/voxels/voxel_renderer_optimized.cpp
#include "../../../include/arena/voxels/voxel_renderer.h"
#include "../../../include/arena/voxels/culling/view_frustum.h"
#include <QOpenGLContext>
#include <QDebug>
#include <algorithm>

// This is an optimized version of the voxel renderer that adds:
// 1. Batched rendering of voxels by material type to minimize state changes
// 2. Sorting by distance for better z-buffer utilization
// 3. More efficient frustum culling

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
    
    // Extract camera position from view matrix (inverse view matrix * origin)
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
    int drawnVoxels = 0;
    
    // First, organize voxels by material type for more efficient rendering
    struct BatchEntry {
        const RenderVoxel* voxel;
        float distanceToCamera;
    };
    
    // Create batches for each material type
    std::map<VoxelType, std::vector<BatchEntry>> batches;
    
    // Group voxels by type and calculate distances
    for (const RenderVoxel& voxel : m_visibleVoxels) {
        // Perform frustum culling if enabled
        if (m_frustumCullingEnabled) {
            QVector3D worldPos = voxel.pos.toWorldPos();
            if (!m_viewFrustum->isPointInside(worldPos)) {
                continue;
            }
        }
        
        // Calculate distance to camera for sorting
        QVector3D worldPos = voxel.pos.toWorldPos();
        float distSq = (worldPos - camPos).lengthSquared();
        
        // Add to appropriate batch
        batches[voxel.type].push_back({&voxel, distSq});
    }
    
    // Sort each batch by distance (front to back for better z-buffer optimization)
    for (auto& batch : batches) {
        std::sort(batch.second.begin(), batch.second.end(), 
                  [](const BatchEntry& a, const BatchEntry& b) {
                      return a.distanceToCamera < b.distanceToCamera;
                  });
    }
    
    // Now render batches in material type order to minimize texture switches
    for (auto& batch : batches) {
        VoxelType type = batch.first;
        
        // Select texture based on voxel type
        QOpenGLTexture* texture = nullptr;
        bool useTexture = true;
        
        switch (type) {
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
        
        // Render all voxels in this batch
        for (const BatchEntry& entry : batch.second) {
            const RenderVoxel* voxel = entry.voxel;
            
            // Set voxel-specific uniforms
            m_shaderProgram->setUniformValue("voxelPosition", voxel->pos.toWorldPos());
            m_shaderProgram->setUniformValue("voxelColor", QVector4D(
                voxel->color.redF(), voxel->color.greenF(), voxel->color.blueF(), voxel->color.alphaF()));
            
            // Draw cube
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
            drawnVoxels++;
        }
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
        qDebug() << "Rendering stats: Drawn voxels:" << drawnVoxels 
                 << "/" << m_visibleVoxels.size() << " (" 
                 << (drawnVoxels * 100 / std::max(1, (int)m_visibleVoxels.size())) << "%)";
    }
}