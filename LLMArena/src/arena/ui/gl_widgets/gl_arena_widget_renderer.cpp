// src/arena/ui/gl_widgets/gl_arena_widget_render.cpp
#include "../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include <QDebug>
#include <QtMath>

// Initialize shader programs
bool GLArenaWidget::initShaders()
{
    // Create billboard shader program
    m_billboardProgram = new QOpenGLShaderProgram();
    
    // Add vertex shader
    if (!m_billboardProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/billboard.vert")) {
        qWarning() << "Failed to compile billboard vertex shader:" << m_billboardProgram->log();
        return false;
    }

// Render grid lines
void GLArenaWidget::renderGrid()
{
    if (!m_gridVAO.isCreated() || m_gridVertexCount == 0) {
        return;
    }
    
    // Bind shader program
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert") ||
        !program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag") ||
        !program.link()) {
        qWarning() << "Failed to create basic shader for grid rendering";
        return;
    }
    
    program.bind();
    
    // Set projection and view matrices
    program.setUniformValue("projection", m_projectionMatrix);
    program.setUniformValue("view", m_viewMatrix);
    
    // Set model matrix (identity for grid)
    QMatrix4x4 model;
    model.setToIdentity();
    program.setUniformValue("model", model);
    
    // Set grid color (light grey)
    program.setUniformValue("objectColor", QVector3D(0.7f, 0.7f, 0.7f));
    
    // Set light position (above grid)
    program.setUniformValue("lightPos", QVector3D(0.0f, 10.0f, 0.0f));
    
    // Set view position for specular lighting
    program.setUniformValue("viewPos", m_playerController->getPosition());
    
    // Bind VAO and draw grid lines
    m_gridVAO.bind();
    glDrawArrays(GL_LINES, 0, m_gridVertexCount);
    m_gridVAO.release();
    
    // Release shader program
    program.release();
}

// Render arena walls
void GLArenaWidget::renderWalls()
{
    if (m_walls.empty()) {
        return;
    }
    
    // Bind shader program
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert") ||
        !program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag") ||
        !program.link()) {
        qWarning() << "Failed to create basic shader for wall rendering";
        return;
    }
    
    program.bind();
    
    // Set projection and view matrices
    program.setUniformValue("projection", m_projectionMatrix);
    program.setUniformValue("view", m_viewMatrix);
    
    // Set model matrix (identity for walls)
    QMatrix4x4 model;
    model.setToIdentity();
    program.setUniformValue("model", model);
    
    // Set wall color (dark grey)
    program.setUniformValue("objectColor", QVector3D(0.3f, 0.3f, 0.3f));
    
    // Set light position (above center of arena)
    program.setUniformValue("lightPos", QVector3D(0.0f, 10.0f, 0.0f));
    
    // Set view position for specular lighting
    program.setUniformValue("viewPos", m_playerController->getPosition());
    
    // Draw each wall
    for (const WallGeometry& wall : m_walls) {
        if (wall.vao && wall.vao->isCreated() && wall.indexCount > 0) {
            wall.vao->bind();
            glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
            wall.vao->release();
        }
    }
    
    // Release shader program
    program.release();
}

// Render character billboards
void GLArenaWidget::renderCharacters()
{
    if (!m_billboardProgram || m_characterSprites.isEmpty()) {
        return;
    }
    
    // Bind billboard shader program
    m_billboardProgram->bind();
    
    // Set projection and view matrices
    m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
    m_billboardProgram->setUniformValue("view", m_viewMatrix);
    
    // Calculate camera up and right vectors from view matrix
    QVector3D cameraRight = QVector3D(m_viewMatrix(0, 0), m_viewMatrix(1, 0), m_viewMatrix(2, 0));
    QVector3D cameraUp = QVector3D(m_viewMatrix(0, 1), m_viewMatrix(1, 1), m_viewMatrix(2, 1));
    
    m_billboardProgram->setUniformValue("cameraRight", cameraRight);
    m_billboardProgram->setUniformValue("cameraUp", cameraUp);
    
    // Draw each character sprite
    for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
        CharacterSprite* sprite = it.value();
        if (sprite && sprite->hasValidTexture() && sprite->hasValidVAO()) {
            // Set billboard position and size
            sprite->render(m_billboardProgram, m_viewMatrix, m_projectionMatrix);
        }
    }
    
    // Release shader program
    m_billboardProgram->release();
}

// Simplified character rendering for fallback
void GLArenaWidget::renderCharactersSimple()
{
    if (!m_billboardProgram || m_characterSprites.isEmpty()) {
        return;
    }
    
    // Bind basic shader program
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert") ||
        !program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag") ||
        !program.link()) {
        qWarning() << "Failed to create basic shader for simplified character rendering";
        return;
    }
    
    program.bind();
    
    // Set projection and view matrices
    program.setUniformValue("projection", m_projectionMatrix);
    program.setUniformValue("view", m_viewMatrix);
    
    // Set light position
    program.setUniformValue("lightPos", QVector3D(0.0f, 10.0f, 0.0f));
    
    // Set view position for specular lighting
    program.setUniformValue("viewPos", m_playerController->getPosition());
    
    // Draw each character sprite as a simple colored cube
    for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
        CharacterSprite* sprite = it.value();
        if (sprite) {
            // Get sprite position
            QVector3D position = sprite->hasValidVAO() ? 
                QVector3D(0, 0, 0) : QVector3D(0, sprite->height() / 2, 0);
                
            // Create model matrix for sprite
            QMatrix4x4 model;
            model.setToIdentity();
            model.translate(position);
            model.scale(sprite->width(), sprite->height(), sprite->depth());
            
            program.setUniformValue("model", model);
            
            // Set color (red by default)
            program.setUniformValue("objectColor", QVector3D(1.0f, 0.0f, 0.0f));
            
            // Draw cube for character
            // This is a placeholder - in a real implementation you'd have a cube VAO
        }
    }
    
    program.release();
}

// Ultimate fallback rendering method - just draw colored quads
void GLArenaWidget::renderCharactersFallback()
{
    // Draw each character as a simple 2D quad
    for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
        const QString& name = it.key();
        CharacterSprite* sprite = it.value();
        
        if (sprite && sprite->hasValidTexture()) {
            // Get position
            QVector3D pos(0, 0, 0); // Get actual position from sprite or game state
            
            // Draw a simple quad
            drawCharacterQuad(sprite->getTexture(), pos.x(), pos.y(), pos.z(), 
                             sprite->width(), sprite->height());
        }
    }
}

// Direct immediate-mode-style quad drawing for absolute fallback
void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, 
                                     float width, float height)
{
    if (!texture || !texture->isCreated()) {
        return;
    }
    
    // This is a simplistic approach for fallback only
    // In a real implementation, you'd want to use proper VAOs and VBOs
    
    // Bind texture
    texture->bind();
    
    // Draw textured quad
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(x - width/2, y, z - height/2);
    glTexCoord2f(1, 0); glVertex3f(x + width/2, y, z - height/2);
    glTexCoord2f(1, 1); glVertex3f(x + width/2, y, z + height/2);
    glTexCoord2f(0, 1); glVertex3f(x - width/2, y, z + height/2);
    glEnd();
    
    // Unbind texture
    texture->release();
}
    
    // Add fragment shader
    if (!m_billboardProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/billboard.frag")) {
        qWarning() << "Failed to compile billboard fragment shader:" << m_billboardProgram->log();
        return false;
    }
    
    // Link shader program
    if (!m_billboardProgram->link()) {
        qWarning() << "Failed to link billboard shader program:" << m_billboardProgram->log();
        return false;
    }
    
    return true;
}

// Render floor quad
void GLArenaWidget::renderFloor()
{
    if (!m_floorVAO.isCreated() || m_floorIndexCount == 0) {
        return;
    }
    
    // Bind shader program
    QOpenGLShaderProgram program;
    if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert") ||
        !program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag") ||
        !program.link()) {
        qWarning() << "Failed to create basic shader for floor rendering";
        return;
    }
    
    program.bind();
    
    // Set projection and view matrices
    program.setUniformValue("projection", m_projectionMatrix);
    program.setUniformValue("view", m_viewMatrix);
    
    // Set model matrix (identity for floor)
    QMatrix4x4 model;
    model.setToIdentity();
    program.setUniformValue("model", model);
    
    // Set floor color (grey)
    program.setUniformValue("objectColor", QVector3D(0.5f, 0.5f, 0.5f));
    
    // Set light position (above floor)
    program.setUniformValue("lightPos", QVector3D(0.0f, 10.0f, 0.0f));
    
    // Set view position for specular lighting
    program.setUniformValue("viewPos", m_playerController->getPosition());
    
    // Bind VAO and draw floor
    m_floorVAO.bind();
    glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
    m_floorVAO.release();
    
    // Release shader program
    program.release();