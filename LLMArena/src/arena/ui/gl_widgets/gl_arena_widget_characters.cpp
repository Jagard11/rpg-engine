// src/arena/ui/gl_widgets/gl_arena_widget_characters.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QImage>
#include <QPainter>

void GLArenaWidget::renderCharacters() {
    // Skip if no shader program
    if (!m_billboardProgram || !m_billboardProgram->isLinked()) {
        return;
    }
    
    try {
        // First attempt to use the normal rendering method
        renderCharactersSimple();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderCharacters:" << e.what();
        
        // Fallback to simpler method if something fails
        try {
            renderCharactersFallback();
        }
        catch (...) {
            qWarning() << "Failed to render characters with fallback method";
        }
    }
    catch (...) {
        qWarning() << "Unknown exception in renderCharacters";
        
        // Fallback to simpler method
        try {
            renderCharactersFallback();
        }
        catch (...) {
            qWarning() << "Failed to render characters with fallback method";
        }
    }
}

void GLArenaWidget::renderCharactersSimple() {
    // Use the billboard program
    m_billboardProgram->bind();
    
    // Set view and projection matrices
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("useTexture", true);
    
    // Render each character sprite
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        CharacterSprite* sprite = it.value();
        
        // Skip invalid sprites
        if (!sprite || !sprite->hasValidTexture() || !sprite->hasValidVAO()) {
            continue;
        }
        
        // Get character position
        QVector3D position = sprite->getPosition();
        
        // Skip if empty position (not placed in world)
        if (position.isNull()) {
            continue;
        }
        
        // Set up billboard model matrix
        QMatrix4x4 modelMatrix;
        
        // Translate to position
        modelMatrix.translate(position);
        
        // Billboard rotation to face camera
        // Extract camera position from inverse view matrix
        QMatrix4x4 invView = m_viewMatrix.inverted();
        QVector3D cameraPos = invView.column(3).toVector3D();
        
        // Calculate direction to camera (just yaw, ignore pitch)
        QVector3D toCamera = cameraPos - position;
        toCamera.setY(0); // Keep billboard upright
        
        if (toCamera.length() > 0.01f) {
            toCamera.normalize();
            
            // Calculate angle
            float angle = atan2(toCamera.z(), toCamera.x());
            
            // Rotate billboard to face camera
            modelMatrix.rotate(90.0f - angle * 180.0f / M_PI, 0, 1, 0);
        }
        
        // Scale to character dimensions
        modelMatrix.scale(sprite->width(), sprite->height(), sprite->depth());
        
        // Set model matrix
        m_billboardProgram->setUniformValue("model", modelMatrix);
        
        // Set default white color
        m_billboardProgram->setUniformValue("color", QVector4D(1, 1, 1, 1));
        
        // Bind texture
        if (sprite->hasValidTexture()) {
            sprite->getTexture()->bind(0);
            m_billboardProgram->setUniformValue("textureSampler", 0);
        }
        
        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Disable depth writing for transparent areas but keep depth testing
        glDepthMask(GL_FALSE);
        
        // Bind VAO and draw
        if (sprite->hasValidVAO()) {
            sprite->getVAO()->bind();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            sprite->getVAO()->release();
        }
        
        // Restore state
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        
        // Release texture
        if (sprite->hasValidTexture()) {
            sprite->getTexture()->release();
        }
    }
    
    // Disable texture usage
    m_billboardProgram->setUniformValue("useTexture", false);
    
    // Unbind program
    m_billboardProgram->release();
}

void GLArenaWidget::renderCharactersFallback() {
    // Use the billboard program
    m_billboardProgram->bind();
    
    // Set view and projection matrices
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    
    // Disable texturing for this simpler method
    m_billboardProgram->setUniformValue("useTexture", false);
    
    // Create a simple colored quad for each character
    for (auto it = m_characterSprites.begin(); it != m_characterSprites.end(); ++it) {
        CharacterSprite* sprite = it.value();
        
        // Get character position (use default if no sprite)
        QVector3D position;
        if (sprite) {
            position = sprite->getPosition();
        } else {
            // Try to get position from game scene
            GameEntity entity = m_gameScene->getEntity(it.key());
            if (!entity.id.isEmpty()) {
                position = entity.position;
            } else {
                continue; // Skip if no position
            }
        }
        
        // Skip if empty position (not placed in world)
        if (position.isNull()) {
            continue;
        }
        
        // Set up model matrix for a simple box representation
        QMatrix4x4 modelMatrix;
        modelMatrix.translate(position);
        
        // Extract camera position from inverse view matrix
        QMatrix4x4 invView = m_viewMatrix.inverted();
        QVector3D cameraPos = invView.column(3).toVector3D();
        
        // Calculate direction to camera
        QVector3D toCamera = cameraPos - position;
        
        // Ensure the box rotates to face camera
        if (toCamera.length() > 0.01f) {
            toCamera.normalize();
            float yaw = atan2(toCamera.z(), toCamera.x());
            modelMatrix.rotate(90.0f - yaw * 180.0f / M_PI, 0, 1, 0);
        }
        
        // Scale to character dimensions (default if no sprite)
        float width = sprite ? sprite->width() : 1.0f;
        float height = sprite ? sprite->height() : 2.0f;
        float depth = sprite ? sprite->depth() : 0.2f;
        
        modelMatrix.scale(width, height, depth);
        
        // Set model matrix
        m_billboardProgram->setUniformValue("model", modelMatrix);
        
        // Use a color based on character name (simple hash)
        int hash = 0;
        for (QChar c : it.key()) {
            hash = (hash * 31 + c.unicode()) & 0xFFFFFF;
        }
        
        // Generate a color from the hash
        QVector4D color(
            ((hash >> 16) & 0xFF) / 255.0f,
            ((hash >> 8) & 0xFF) / 255.0f,
            (hash & 0xFF) / 255.0f,
            1.0f
        );
        
        // Set color
        m_billboardProgram->setUniformValue("color", color);
        
        // Draw a simple quad directly
        static const GLfloat quadVertices[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f
        };
        
        // Use a temporary VBO for the quad
        QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
        vbo.create();
        vbo.bind();
        vbo.allocate(quadVertices, sizeof(quadVertices));
        
        // Set up vertex attributes - just position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
        
        // Disable unused attributes
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);
        
        // Draw the quad
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        
        // Clean up
        vbo.release();
        vbo.destroy();
    }
    
    // Enable all attributes again
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    
    // Unbind program
    m_billboardProgram->release();
}

void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height) {
    // Set up model matrix
    QMatrix4x4 modelMatrix;
    modelMatrix.translate(x, y, z);
    
    // Extract camera position from view matrix
    QMatrix4x4 invView = m_viewMatrix.inverted();
    QVector3D cameraPos = invView.column(3).toVector3D();
    
    // Calculate rotation to face camera
    QVector3D toCamera = cameraPos - QVector3D(x, y, z);
    toCamera.setY(0); // Keep billboard upright
    
    if (toCamera.length() > 0.01f) {
        toCamera.normalize();
        float angle = atan2(toCamera.z(), toCamera.x());
        modelMatrix.rotate(90.0f - angle * 180.0f / M_PI, 0, 1, 0);
    }
    
    // Scale to desired size
    modelMatrix.scale(width, height, 1.0f);
    
    // Set model matrix
    m_billboardProgram->setUniformValue("model", modelMatrix);
    
    // Set white color for texturing
    m_billboardProgram->setUniformValue("color", QVector4D(1, 1, 1, 1));
    
    // Use texture if provided
    if (texture && texture->isCreated()) {
        m_billboardProgram->setUniformValue("useTexture", true);
        texture->bind(0);
        m_billboardProgram->setUniformValue("textureSampler", 0);
    } else {
        m_billboardProgram->setUniformValue("useTexture", false);
    }
    
    // Define vertices for a quad
    static const GLfloat quadVertices[] = {
        // Position (3) + normal (3) + texcoord (2)
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f
    };
    
    // Create and bind a temporary VBO
    QOpenGLBuffer vbo(QOpenGLBuffer::VertexBuffer);
    vbo.create();
    vbo.bind();
    vbo.allocate(quadVertices, sizeof(quadVertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0); // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), nullptr);
    
    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 
                         reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    
    glEnableVertexAttribArray(2); // texcoord
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 
                         reinterpret_cast<void*>(6 * sizeof(GLfloat)));
    
    // Enable blending for transparent textures
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    // Restore state
    glDisable(GL_BLEND);
    
    // Release resources
    if (texture && texture->isCreated()) {
        texture->release();
    }
    
    vbo.release();
    vbo.destroy();
    
    // Reset texture usage
    m_billboardProgram->setUniformValue("useTexture", false);
}