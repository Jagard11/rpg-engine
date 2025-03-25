#include "../../../include/arena/voxels/voxel_renderer.h"
#include "../../../include/arena/voxels/culling/view_frustum.h"
#include <QOpenGLContext>
#include <QDebug>
#include <algorithm>

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
    
    // Setup lighting
    QVector3D lightPos(0.0f, 10.0f, 0.0f); // Light above center
    m_shaderProgram->setUniformValue("lightPos", lightPos);
    
    // Bind VAO
    m_vao.bind();
    
    // Enable texture unit 0
    glActiveTexture(GL_TEXTURE0);
    m_shaderProgram->setUniformValue("textureSampler", 0);
    
    // Track current bound texture to avoid redundant binds
    GLuint currentTexture = 0;
    
    // Draw each visible voxel
    for (const RenderVoxel& voxel : m_visibleVoxels) {
        // Skip voxels outside the view frustum
        if (m_frustumCullingEnabled) {
            QVector3D worldPos = voxel.pos.toWorldPos();
            // Use sphere test instead of point test with a larger radius
            float radius = 1.0f; // Increased from 0.866f to 1.0f for better visibility
            if (!m_viewFrustum->isSphereInside(worldPos, radius)) {
                continue;
            }
        }
        
        // Set voxel-specific uniforms
        m_shaderProgram->setUniformValue("voxelPosition", voxel.pos.toWorldPos());
        m_shaderProgram->setUniformValue("voxelColor", QVector4D(
            voxel.color.redF(), voxel.color.greenF(), voxel.color.blueF(), voxel.color.alphaF()));
        
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
        
        // Draw cube
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
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
} 