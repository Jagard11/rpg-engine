// src/arena/debug/console/debug_console_render.cpp
#include "../../../../include/arena/debug/console/debug_console.h"
#include <QOpenGLContext>
#include <QPainter>
#include <QApplication>
#include <QWidget>
#include <QPaintDevice>
#include <QDebug>

// This file contains the rendering functions for the debug console
// Keeping these functions separate helps maintain modularity

// Make sure we register the quintptr type for QVariant usage
Q_DECLARE_METATYPE(quintptr)

void DebugConsole::drawConsoleText(int screenWidth, int screenHeight, float consoleHeight) {
    // Get the rendering target widget using the safer quintptr approach
    quintptr widgetPtr = property("render_widget").value<quintptr>();
    
    if (widgetPtr == 0) {
        qWarning() << "Debug console has no render widget target - widget pointer is null";
        return;
    }
    
    // Directly reinterpret_cast from quintptr to QWidget*
    QWidget* widget = reinterpret_cast<QWidget*>(widgetPtr);
    
    if (!widget) {
        qWarning() << "Debug console has invalid render widget target";
        return;
    }
    
    if (!widget->isVisible()) {
        qWarning() << "Debug console render widget is not visible";
        // Continue anyway - it might just not be visible yet
    }
    
    // Make sure the OpenGL context is current
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        qWarning() << "No current OpenGL context in drawConsoleText";
        return;
    }
    
    // Flush GL commands to ensure rendering is complete before painting
    context->functions()->glFlush();
    
    // Initialize QPainter for overlay drawing with explicit cast to QPaintDevice
    QPainter painter(static_cast<QPaintDevice*>(widget));
    
    if (!painter.isActive()) {
        qWarning() << "Failed to activate QPainter on widget";
        return;
    }
    
    // Set up font and color
    painter.setFont(m_consoleFont);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Draw semi-transparent background for better readability
    QColor bgColor(0, 0, 0, 180); // Black with 70% opacity
    painter.fillRect(0, 0, screenWidth, consoleHeight, bgColor);
    
    // Draw console title
    painter.setPen(Qt::yellow);
    painter.drawText(10, 20, "Debug Console [~]");
    
    // Draw output lines
    int lineHeight = 20;
    int outputY = consoleHeight - 60; // Start above input line
    
    for (int i = m_outputLines.size() - 1; i >= 0 && outputY > 30; --i) {
        painter.setPen(Qt::white);
        painter.drawText(10, outputY, m_outputLines[i]);
        outputY -= lineHeight;
    }
    
    // Draw input line with blinking cursor
    painter.setPen(Qt::green);
    // Add blinking cursor based on time
    static bool showCursor = true;
    static int cursorCounter = 0;
    
    cursorCounter = (cursorCounter + 1) % 30; // Toggle every ~0.5 seconds (assuming 60 fps)
    if (cursorCounter == 0) {
        showCursor = !showCursor;
    }
    
    QString cursorText = showCursor ? "_" : " ";
    painter.drawText(10, consoleHeight - 30, "> " + m_inputText + cursorText);
    
    // End painting
    painter.end();
}