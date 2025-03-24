// src/arena/debug/console/debug_console_render.cpp
#include "../../../../include/arena/debug/console/debug_console.h"
#include <QOpenGLContext>
#include <QPainter>
#include <QApplication>
#include <QWidget>

// This file contains the rendering functions for the debug console
// Keeping these functions separate helps maintain modularity

// Forward declaration for new render method
void DebugConsole::drawConsoleText(int screenWidth, int screenHeight, float consoleHeight);

void DebugConsole::render(int screenWidth, int screenHeight) {
    if (!m_visible) {
        return;
    }
    
    // Make sure we have a valid shader
    if (!m_consoleShader || !m_consoleShader->isLinked() || !m_quadVAO.isCreated()) {
        qWarning() << "Debug console not properly initialized";
        return;
    }
    
    // Calculate console dimensions
    float consoleHeight = screenHeight * m_consoleHeight;
    
    // Save OpenGL state
    GLint oldBlend;
    glGetIntegerv(GL_BLEND, &oldBlend);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable depth testing for UI
    GLint oldDepthTest;
    glGetIntegerv(GL_DEPTH_TEST, &oldDepthTest);
    glDisable(GL_DEPTH_TEST);
    
    // Bind shader
    m_consoleShader->bind();
    
    // Set up orthographic projection
    QMatrix4x4 projection;
    projection.ortho(0, screenWidth, screenHeight, 0, -1, 1); // Origin at top-left
    m_consoleShader->setUniformValue("projection", projection);
    
    // Draw console background
    m_quadVAO.bind();
    
    // Set background color (semi-transparent black)
    m_consoleShader->setUniformValue("color", QVector4D(0.1f, 0.1f, 0.2f, m_consoleOpacity));
    
    // Set model matrix for background
    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(0, 0, 0);
    model.scale(screenWidth, consoleHeight, 1);
    m_consoleShader->setUniformValue("model", model);
    
    // Draw background quad
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    
    m_quadVAO.release();
    m_consoleShader->release();
    
    // Draw console text
    drawConsoleText(screenWidth, screenHeight, consoleHeight);
    
    // Restore OpenGL state
    if (oldBlend) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    
    if (oldDepthTest) {
        glEnable(GL_DEPTH_TEST);
    }
}

void DebugConsole::drawConsoleText(int screenWidth, int screenHeight, float consoleHeight) {
    // Get the rendering target widget
    quintptr widgetPtr = property("render_widget").value<quintptr>();
    QWidget* widget = reinterpret_cast<QWidget*>(static_cast<void*>(widgetPtr));
    
    if (!widget) {
        return;
    }
    
    // Make sure the OpenGL context is current
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        return;
    }
    
    // Flush GL commands to ensure rendering is complete before painting
    context->functions()->glFlush();
    
    // Initialize QPainter for overlay drawing
    QPainter painter(widget);
    
    if (!painter.isActive()) {
        return;
    }
    
    // Set up font and color
    painter.setFont(m_consoleFont);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Draw output lines
    int lineHeight = 20;
    int outputY = consoleHeight - 60; // Start above input line
    
    for (int i = m_outputLines.size() - 1; i >= 0 && outputY > 0; --i) {
        painter.setPen(Qt::white);
        painter.drawText(10, outputY, m_outputLines[i]);
        outputY -= lineHeight;
    }
    
    // Draw input line
    painter.setPen(Qt::green);
    painter.drawText(10, consoleHeight - 30, "> " + m_inputText + "_");
    
    // End painting
    painter.end();
}