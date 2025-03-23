// src/arena/voxels/voxel_renderer_with_player.cpp
#include "../../../include/arena/voxels/voxel_renderer.h"
#include "../../../include/arena/player/player_entity.h"
#include <QOpenGLContext>
#include <QDebug>
#include <algorithm>

/**
 * Modified version of the VoxelRenderer that specifically uses the PlayerEntity
 * for frustum culling
 */

// Add a private member to VoxelRenderer
// PlayerEntity* m_playerEntity = nullptr;

// Add setter method to VoxelRenderer
// void VoxelRenderer::setPlayerEntity(PlayerEntity* playerEntity) {
//     m_playerEntity = playerEntity;
// }

// Modified render method that uses the player entity for culling
void VoxelRenderer::renderWithPlayer(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix, PlayerEntity* playerEntity) {
    if (!m_world || !m_shaderProgram || !playerEntity) return;
    
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
    
    // Extract camera position from player
    QVector3D camPos = playerEntity->getPosition();
    m_shaderProgram->setUniformValue("viewPos", camPos);
    
    // Setup lighting
    QVector3D lightPos(0.0f, 100.0f, 0.0f); // Light from above
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
    
    // Keep count of drawn and culled voxels for debugging
    int drawnVoxels = 0;
    int culledVoxels = 0;
    
    // First, organize voxels by material type for more efficient rendering
    struct BatchEntry {
        const RenderVoxel* voxel;
        float distanceToCamera;
    };
    
    // Create batches for each material type
    std::map<VoxelType, std::vector<BatchEntry>> batches;
    
    // Group voxels by type and calculate distances
    for (const RenderVoxel& voxel : m_visibleVoxels) {
        // Get world position of voxel
        QVector3D worldPos = voxel.pos.toWorldPos();
        
        // Perform frustum culling if enabled
        if (m_frustumCullingEnabled) {
            // Create a bounding sphere for the voxel
            float radius = 0.866f; // Radius of bounding sphere for a unit cube (sqrt(3)/2)
            
            // Use the player entity's camera for culling
            if (!playerEntity->isSphereVisible(worldPos, radius)) {
                culledVoxels++;
                continue; // Skip this voxel if it's outside the frustum
            }
        }
        
        // Calculate squared distance to camera for sorting
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
        
        // Skip empty batches
        if (batch.second.empty()) {
            continue;
        }
        
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
                 << (m_visibleVoxels.size() > 0 ? (drawnVoxels * 100 / m_visibleVoxels.size()) : 0) << "%),"
                 << "Culled:" << culledVoxels;
    }
}