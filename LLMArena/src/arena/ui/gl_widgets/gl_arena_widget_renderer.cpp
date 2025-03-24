// src/arena/ui/gl_widgets/gl_arena_widget_renderer.cpp
#include "../../../../include/arena/ui/gl_widgets/gl_arena_widget.h"
#include "../../../../include/arena/debug/debug_system.h"
#include <QDebug>
#include <QOpenGLShaderProgram>

// Initialize shaders
bool GLArenaWidget::initShaders()
{
    try {
        // Create shader program for billboards
        m_billboardProgram = new QOpenGLShaderProgram(this);
        
        // Load and compile shaders
        if (!m_billboardProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/billboard.vert")) {
            qCritical() << "Failed to compile billboard vertex shader:" << m_billboardProgram->log();
            return false;
        }
        
        if (!m_billboardProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/billboard.frag")) {
            qCritical() << "Failed to compile billboard fragment shader:" << m_billboardProgram->log();
            return false;
        }
        
        // Link shader program
        if (!m_billboardProgram->link()) {
            qCritical() << "Failed to link billboard shader program:" << m_billboardProgram->log();
            return false;
        }
        
        qDebug() << "Shaders initialized successfully";
        return true;
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in initShaders:" << e.what();
        return false;
    }
    catch (...) {
        qCritical() << "Unknown exception in initShaders";
        return false;
    }
}

// Render grid
void GLArenaWidget::renderGrid()
{
    if (!m_gridVAO.isCreated() || !m_gridVBO.isCreated()) {
        return;
    }
    
    try {
        // Use a simple shader for grid
        QOpenGLShaderProgram program;
        if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert")) {
            qWarning() << "Failed to load grid vertex shader";
            return;
        }
        
        if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag")) {
            qWarning() << "Failed to load grid fragment shader";
            return;
        }
        
        if (!program.link()) {
            qWarning() << "Failed to link grid shader program";
            return;
        }
        
        // Bind shader and set uniforms
        program.bind();
        program.setUniformValue("model", QMatrix4x4());
        program.setUniformValue("view", m_viewMatrix);
        program.setUniformValue("projection", m_projectionMatrix);
        program.setUniformValue("objectColor", QVector3D(0.5f, 0.5f, 0.5f));
        
        // Bind VAO and draw grid
        m_gridVAO.bind();
        glDrawArrays(GL_LINES, 0, m_gridVertexCount);
        m_gridVAO.release();
        
        // Release shader
        program.release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderGrid:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in renderGrid";
    }
}

// Render walls
void GLArenaWidget::renderWalls()
{
    if (m_walls.empty()) {
        return;
    }
    
    try {
        // Use a simple shader for walls
        QOpenGLShaderProgram program;
        if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert")) {
            qWarning() << "Failed to load wall vertex shader";
            return;
        }
        
        if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag")) {
            qWarning() << "Failed to load wall fragment shader";
            return;
        }
        
        if (!program.link()) {
            qWarning() << "Failed to link wall shader program";
            return;
        }
        
        // Bind shader and set common uniforms
        program.bind();
        program.setUniformValue("view", m_viewMatrix);
        program.setUniformValue("projection", m_projectionMatrix);
        program.setUniformValue("objectColor", QVector3D(0.8f, 0.8f, 0.8f));
        
        // Draw each wall
        for (const auto& wall : m_walls) {
            if (wall.vao && wall.vao->isCreated() && wall.ibo && wall.indexCount > 0) {
                program.setUniformValue("model", QMatrix4x4());
                
                // Bind VAO and draw
                wall.vao->bind();
                glDrawElements(GL_TRIANGLES, wall.indexCount, GL_UNSIGNED_INT, nullptr);
                wall.vao->release();
            }
        }
        
        // Release shader
        program.release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderWalls:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in renderWalls";
    }
}

// Render characters
void GLArenaWidget::renderCharacters()
{
    if (m_characterSprites.isEmpty() || !m_billboardProgram || !m_billboardProgram->isLinked()) {
        renderCharactersFallback();
        return;
    }
    
    try {
        // Bind shader and set common uniforms
        m_billboardProgram->bind();
        
        // Camera right vector (from view matrix)
        QVector3D cameraRight(m_viewMatrix(0, 0), m_viewMatrix(0, 1), m_viewMatrix(0, 2));
        
        // Camera up vector (from view matrix)
        QVector3D cameraUp(m_viewMatrix(1, 0), m_viewMatrix(1, 1), m_viewMatrix(1, 2));
        
        // Set common uniforms
        m_billboardProgram->setUniformValue("view", m_viewMatrix);
        m_billboardProgram->setUniformValue("projection", m_projectionMatrix);
        m_billboardProgram->setUniformValue("cameraRight", cameraRight);
        m_billboardProgram->setUniformValue("cameraUp", cameraUp);
        
        // Render each character sprite
        for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
            CharacterSprite* sprite = it.value();
            if (sprite && sprite->hasValidTexture() && sprite->hasValidVAO()) {
                sprite->render(m_billboardProgram, m_viewMatrix, m_projectionMatrix);
            }
        }
        
        // Release shader
        m_billboardProgram->release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderCharacters:" << e.what();
        renderCharactersSimple();
    }
    catch (...) {
        qWarning() << "Unknown exception in renderCharacters";
        renderCharactersSimple();
    }
}

// Simple character rendering for fallback
void GLArenaWidget::renderCharactersSimple()
{
    if (m_characterSprites.isEmpty()) {
        return;
    }
    
    try {
        // Use a simple shader for characters
        QOpenGLShaderProgram program;
        if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert")) {
            qWarning() << "Failed to load character vertex shader";
            renderCharactersFallback();
            return;
        }
        
        if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag")) {
            qWarning() << "Failed to load character fragment shader";
            renderCharactersFallback();
            return;
        }
        
        if (!program.link()) {
            qWarning() << "Failed to link character shader program";
            renderCharactersFallback();
            return;
        }
        
        // Bind shader and set common uniforms
        program.bind();
        program.setUniformValue("view", m_viewMatrix);
        program.setUniformValue("projection", m_projectionMatrix);
        
        // Draw each character as a simple cube
        for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
            CharacterSprite* sprite = it.value();
            if (sprite) {
                QVector3D position = QVector3D(0, 0, 0); // Get actual position
                QVector3D size = QVector3D(1, 2, 1); // Simple character size
                
                // Draw a colored cube for each character
                program.setUniformValue("objectColor", QVector3D(0.0f, 0.8f, 0.8f));
                
                // TODO: Draw simple cube
            }
        }
        
        // Release shader
        program.release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderCharactersSimple:" << e.what();
        renderCharactersFallback();
    }
    catch (...) {
        qWarning() << "Unknown exception in renderCharactersSimple";
        renderCharactersFallback();
    }
}

// Absolute fallback rendering method for characters
void GLArenaWidget::renderCharactersFallback()
{
    // Direct rendering without complex shader/VAO setup
    for (auto it = m_characterSprites.constBegin(); it != m_characterSprites.constEnd(); ++it) {
        CharacterSprite* sprite = it.value();
        if (sprite && sprite->hasValidTexture()) {
            // Draw a basic quad for this character
            drawCharacterQuad(sprite->getTexture(), 0, 0, 0, sprite->width(), sprite->height());
        }
    }
}

// Direct quad drawing without VAOs
void GLArenaWidget::drawCharacterQuad(QOpenGLTexture* texture, float x, float y, float z, float width, float height)
{
    if (!texture || !texture->isCreated()) {
        return;
    }
    
    try {
        // Draw a textured quad directly using OpenGL
        texture->bind();
        
        // Draw quad
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex3f(x, y, z);
        glTexCoord2f(1, 0); glVertex3f(x + width, y, z);
        glTexCoord2f(1, 1); glVertex3f(x + width, y + height, z);
        glTexCoord2f(0, 1); glVertex3f(x, y + height, z);
        glEnd();
        
        texture->release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in drawCharacterQuad:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in drawCharacterQuad";
    }
}

// Render floor
void GLArenaWidget::renderFloor()
{
    if (!m_floorVAO.isCreated() || !m_floorVBO.isCreated() || !m_floorIBO.isCreated() || m_floorIndexCount == 0) {
        return;
    }
    
    try {
        // Use a simple shader for floor
        QOpenGLShaderProgram program;
        if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/basic.vert")) {
            qWarning() << "Failed to load floor vertex shader";
            return;
        }
        
        if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/basic.frag")) {
            qWarning() << "Failed to load floor fragment shader";
            return;
        }
        
        if (!program.link()) {
            qWarning() << "Failed to link floor shader program";
            return;
        }
        
        // Bind shader and set uniforms
        program.bind();
        program.setUniformValue("model", QMatrix4x4());
        program.setUniformValue("view", m_viewMatrix);
        program.setUniformValue("projection", m_projectionMatrix);
        program.setUniformValue("objectColor", QVector3D(0.3f, 0.3f, 0.3f));
        
        // Bind VAO and draw floor
        m_floorVAO.bind();
        glDrawElements(GL_TRIANGLES, m_floorIndexCount, GL_UNSIGNED_INT, nullptr);
        m_floorVAO.release();
        
        // Release shader
        program.release();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in renderFloor:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in renderFloor";
    }
}