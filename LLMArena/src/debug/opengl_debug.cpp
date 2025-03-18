// src/debug/opengl_debug.cpp
#include "../../include/debug/opengl_debug.h"
#include <QDir>
#include <QThread>

// Static member initialization
bool OpenGLDebug::s_enabled = true;
int OpenGLDebug::s_verboseLevel = 2;
bool OpenGLDebug::s_fileLogging = false;
QFile OpenGLDebug::s_logFile;
int OpenGLDebug::s_frameCount = 0;
int OpenGLDebug::s_indentLevel = 0;

void OpenGLDebug::init(bool enableFileLogging) {
    s_enabled = true;
    s_fileLogging = enableFileLogging;
    
    if (s_fileLogging) {
        QString logDir = QDir::homePath() + "/.oobabooga_rpg/logs";
        QDir().mkpath(logDir);
        
        QString logPath = logDir + "/opengl_" + 
                         QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".log";
        
        s_logFile.setFileName(logPath);
        if (!s_logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to open OpenGL debug log file:" << logPath;
            s_fileLogging = false;
        } else {
            QTextStream stream(&s_logFile);
            stream << "=== OpenGL Debug Log ===" << Qt::endl;
            stream << "Started: " << QDateTime::currentDateTime().toString() << Qt::endl;
            stream << "=======================\n" << Qt::endl;
        }
    }
    
    logInfo("OpenGL debug initialized");
}

bool OpenGLDebug::checkGLError(const QString& location) {
    if (!s_enabled) return false;
    
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context || !context->isValid()) {
        logError(location + ": No valid OpenGL context");
        return true;
    }
    
    QOpenGLFunctions* f = context->functions();
    if (!f) {
        logError(location + ": Failed to get OpenGL functions");
        return true;
    }
    
    GLenum err = f->glGetError();
    if (err != GL_NO_ERROR) {
        QString errorString;
        switch (err) {
            case GL_INVALID_ENUM:
                errorString = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                errorString = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                errorString = "GL_INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                errorString = "GL_STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                errorString = "GL_STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                errorString = "GL_OUT_OF_MEMORY";
                break;
            default:
                errorString = "Unknown error: " + QString::number(err);
                break;
        }
        
        logError(location + ": OpenGL error: " + errorString);
        return true;
    }
    
    return false;
}

void OpenGLDebug::logDebug(const QString& message) {
    if (!s_enabled || s_verboseLevel < 3) return;
    
    QString indentation(s_indentLevel * 2, ' ');
    QString logMessage = "DEBUG: " + indentation + message;
    
    qDebug().noquote() << logMessage;
    
    if (s_fileLogging && s_logFile.isOpen()) {
        QTextStream stream(&s_logFile);
        stream << logMessage << Qt::endl;
    }
}

void OpenGLDebug::logInfo(const QString& message) {
    if (!s_enabled || s_verboseLevel < 2) return;
    
    QString indentation(s_indentLevel * 2, ' ');
    QString logMessage = "INFO: " + indentation + message;
    
    qInfo().noquote() << logMessage;
    
    if (s_fileLogging && s_logFile.isOpen()) {
        QTextStream stream(&s_logFile);
        stream << logMessage << Qt::endl;
    }
}

void OpenGLDebug::logWarning(const QString& message) {
    if (!s_enabled || s_verboseLevel < 1) return;
    
    QString indentation(s_indentLevel * 2, ' ');
    QString logMessage = "WARNING: " + indentation + message;
    
    qWarning().noquote() << logMessage;
    
    if (s_fileLogging && s_logFile.isOpen()) {
        QTextStream stream(&s_logFile);
        stream << logMessage << Qt::endl;
    }
}

void OpenGLDebug::logError(const QString& message) {
    if (!s_enabled) return;
    
    QString indentation(s_indentLevel * 2, ' ');
    QString logMessage = "ERROR: " + indentation + message;
    
    qCritical().noquote() << logMessage;
    
    if (s_fileLogging && s_logFile.isOpen()) {
        QTextStream stream(&s_logFile);
        stream << logMessage << Qt::endl;
    }
}

void OpenGLDebug::beginFrame() {
    if (!s_enabled || s_verboseLevel < 3) return;
    
    s_frameCount++;
    logDebug("--- Begin Frame " + QString::number(s_frameCount) + " ---");
}

void OpenGLDebug::endFrame() {
    if (!s_enabled || s_verboseLevel < 3) return;
    
    logDebug("--- End Frame " + QString::number(s_frameCount) + " ---");
    
    // Flush log file if we're logging to file
    if (s_fileLogging && s_logFile.isOpen()) {
        s_logFile.flush();
    }
}

void OpenGLDebug::beginOperation(const QString& name) {
    if (!s_enabled || s_verboseLevel < 3) return;
    
    logDebug("Begin: " + name);
    s_indentLevel++;
}

void OpenGLDebug::endOperation(const QString& name) {
    if (!s_enabled || s_verboseLevel < 3) return;
    
    if (s_indentLevel > 0) s_indentLevel--;
    logDebug("End: " + name);
}

void OpenGLDebug::logMemoryAllocated(const QString& resourceType, size_t bytes) {
    if (!s_enabled || s_verboseLevel < 2) return;
    
    logInfo("Allocated " + QString::number(bytes) + " bytes for " + resourceType);
}

void OpenGLDebug::logMemoryFreed(const QString& resourceType, size_t bytes) {
    if (!s_enabled || s_verboseLevel < 2) return;
    
    logInfo("Freed " + QString::number(bytes) + " bytes from " + resourceType);
}

QString OpenGLDebug::pointerInfo(const void* ptr) {
    if (!ptr) return "nullptr";
    
    QString info = QString("0x%1").arg((quintptr)ptr, QT_POINTER_SIZE * 2, 16, QChar('0'));
    return info;
}

QString OpenGLDebug::currentGLError() {
    QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context || !context->isValid()) {
        return "No valid OpenGL context";
    }
    
    QOpenGLFunctions* f = context->functions();
    if (!f) {
        return "Failed to get OpenGL functions";
    }
    
    GLenum err = f->glGetError();
    if (err == GL_NO_ERROR) {
        return "No error";
    }
    
    switch (err) {
        case GL_INVALID_ENUM:
            return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:
            return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:
            return "GL_INVALID_OPERATION";
        case GL_STACK_OVERFLOW:
            return "GL_STACK_OVERFLOW";
        case GL_STACK_UNDERFLOW:
            return "GL_STACK_UNDERFLOW";
        case GL_OUT_OF_MEMORY:
            return "GL_OUT_OF_MEMORY";
        default:
            return "Unknown error: " + QString::number(err);
    }
}

bool OpenGLDebug::isContextValid(QOpenGLContext* context) {
    if (!context) {
        logError("Null OpenGL context");
        return false;
    }
    
    if (!context->isValid()) {
        logError("Invalid OpenGL context");
        return false;
    }
    
    return true;
}

void OpenGLDebug::setEnabled(bool enabled) {
    s_enabled = enabled;
}

void OpenGLDebug::setVerboseLevel(int level) {
    s_verboseLevel = level;
}