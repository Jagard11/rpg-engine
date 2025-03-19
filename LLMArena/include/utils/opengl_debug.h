// include/debug/opengl_debug.h
#ifndef OPENGL_DEBUG_H
#define OPENGL_DEBUG_H

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QString>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QTextStream>

class OpenGLDebug {
public:
    // Initialize debug system
    static void init(bool enableFileLogging = false);
    
    // Log GL errors 
    static bool checkGLError(const QString& location);
    
    // Debug logging with severity levels
    static void logDebug(const QString& message);
    static void logInfo(const QString& message);
    static void logWarning(const QString& message);
    static void logError(const QString& message);
    
    // Frame tracking
    static void beginFrame();
    static void endFrame();
    
    // Operation tracking
    static void beginOperation(const QString& name);
    static void endOperation(const QString& name);
    
    // Memory tracking
    static void logMemoryAllocated(const QString& resourceType, size_t bytes);
    static void logMemoryFreed(const QString& resourceType, size_t bytes);
    
    // Get pointer info
    static QString pointerInfo(const void* ptr);
    
    // Force an error check in current context and return as string
    static QString currentGLError();
    
    // Check if context is valid
    static bool isContextValid(QOpenGLContext* context);
    
    // Configure debug output
    static void setEnabled(bool enabled);
    static void setVerboseLevel(int level);

private:
    static bool s_enabled;
    static int s_verboseLevel;
    static bool s_fileLogging;
    static QFile s_logFile;
    static int s_frameCount;
    static int s_indentLevel;
};

// Helper macros for common operations
#define GL_DEBUG_MARKER(name) \
    OpenGLDebug::beginOperation(name); \
    OpenGLDebug::checkGLError("Before " name);

#define GL_DEBUG_END_MARKER(name) \
    OpenGLDebug::checkGLError("After " name); \
    OpenGLDebug::endOperation(name);

#define GL_DEBUG_OBJECT(ptr) \
    OpenGLDebug::pointerInfo(ptr)

#define GL_CHECK_ERROR(location) \
    OpenGLDebug::checkGLError(location)

#define GL_CONTEXT_VALID(ctx) \
    OpenGLDebug::isContextValid(ctx)

#endif // OPENGL_DEBUG_H