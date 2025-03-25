// src/arena/debug/console/debug_console_render.cpp
#include "../../../../include/arena/debug/console/debug_console.h"
#include <QOpenGLContext>
#include <QPainter>
#include <QApplication>
#include <QWidget>
#include <QPaintDevice>
#include <QDebug>

// Register the quintptr type for QVariant usage
Q_DECLARE_METATYPE(quintptr)

void DebugConsole::drawConsoleText(int screenWidth, int screenHeight, float consoleHeight) {
    // Get the rendering target widget using the safer quintptr approach
    QVariant variantWidget = property("render_widget");
    if (!variantWidget.isValid()) {
        qWarning() << "Debug console has no valid render_widget property";
        return;
    }
    
    quintptr widgetPtr = variantWidget.value<quintptr>();
    if (widgetPtr == 0) {
        qWarning() << "Debug console render_widget is null";
        return;
    }
    
    // Directly reinterpret_cast from quintptr to QWidget*
    QWidget* widget = reinterpret_cast<QWidget*>(widgetPtr);
    if (!widget) {
        qWarning() << "Debug console has invalid render widget target";
        return;
    }
    
    // Make sure the OpenGL context is current
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context || !context->isValid()) {
        qWarning() << "No valid OpenGL context in drawConsoleText";
        return;
    }
    
    // Calculate proper console dimensions
    float actualConsoleHeight = screenHeight * m_consoleHeight;
    
    // Flush GL commands to ensure rendering is complete before painting
    context->functions()->glFlush();
    
    // Initialize QPainter for overlay drawing
    // We need to ensure the widget is actually a valid QPaintDevice
    QPaintDevice* paintDevice = widget;
    if (!paintDevice) {
        qWarning() << "Failed to get valid QPaintDevice from widget";
        return;
    }
    
    QPainter painter(paintDevice);
    if (!painter.isActive()) {
        qWarning() << "Failed to activate QPainter on widget";
        return;
    }
    
    // Log that we're drawing the console
    qDebug() << "Drawing debug console with dimensions:" << screenWidth << "x" << actualConsoleHeight;
    
    // Set up font and color
    painter.setFont(m_consoleFont);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Draw semi-transparent background for better readability
    QColor bgColor(0, 0, 0, 220); // Black with high opacity
    painter.fillRect(0, 0, screenWidth, actualConsoleHeight, bgColor);
    
    // Draw border at the bottom of the console
    painter.setPen(QColor(255, 255, 255, 128)); // Semi-transparent white
    painter.drawLine(0, actualConsoleHeight, screenWidth, actualConsoleHeight);
    
    // Draw console title
    painter.setPen(Qt::yellow);
    painter.drawText(10, 20, "Debug Console [~]");
    
    // Draw output lines
    int lineHeight = QFontMetrics(m_consoleFont).height() + 2; // Add some spacing
    int outputY = actualConsoleHeight - lineHeight - 30; // Start above input line
    
    // Draw most recent lines first (from bottom of console upward)
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
    
    cursorCounter = (cursorCounter + 1) % 20; // Toggle more quickly
    if (cursorCounter == 0) {
        showCursor = !showCursor;
    }
    
    QString cursorText = showCursor ? "_" : " ";
    painter.drawText(10, actualConsoleHeight - 10, "> " + m_inputText + cursorText);
    
    // End painting
    painter.end();
}