// src/arena/debug/console/debug_console.cpp
#include "../../../include/arena/debug/console/debug_console.h"
#include "../../../include/arena/debug/commands/debug_command.h"
#include "../../../include/arena/game/player_controller.h"
#include "../../../include/arena/core/arena_core.h"

#include <QDebug>
#include <QPainter>
#include <QDateTime>
#include <QKeyEvent>
#include <QFontDatabase>

DebugConsole::DebugConsole(GameScene* scene, PlayerController* player, QObject* parent)
    : QObject(parent),
      m_gameScene(scene),
      m_playerController(player),
      m_visible(false),
      m_historyIndex(-1),
      m_consoleShader(nullptr),
      m_maxOutputLines(15),
      m_consoleHeight(0.4f),  // 40% of screen height
      m_consoleOpacity(0.8f)  // 80% opacity
{
    // Initialize console font
    int fontId = QFontDatabase::addApplicationFont(":/fonts/consolas.ttf");
    if (fontId != -1) {
        QString fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
        m_consoleFont = QFont(fontFamily, 12);
    } else {
        // Fallback to monospace font
        m_consoleFont = QFont("Monospace", 12);
        m_consoleFont.setStyleHint(QFont::Monospace);
    }
    
    // Add initial welcome message
    addOutput("== Debug Console ==");
    addOutput("Type 'help' for available commands");
}

DebugConsole::~DebugConsole() {
    // Clean up OpenGL resources
    if (m_quadVBO.isCreated()) {
        m_quadVBO.destroy();
    }
    
    if (m_quadVAO.isCreated()) {
        m_quadVAO.destroy();
    }
    
    delete m_consoleShader;
    
    // Delete commands
    for (auto command : m_commands) {
        delete command;
    }
    m_commands.clear();
}

void DebugConsole::initialize() {
    // Initialize OpenGL resources
    initializeGL();
}

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
    
    // Draw console text using QPainter
    QPainter painter(this->parent()->property("qpainter_target").value<QPaintDevice*>());
    if (painter.isActive()) {
        painter.setFont(m_consoleFont);
        
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
        
        painter.end();
    }
    
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

bool DebugConsole::handleKeyPress(int key, const QString& text) {
    // Toggle console visibility with backtick key
    if (key == Qt::Key_QuoteLeft) {
        setVisible(!m_visible);
        return true;
    }
    
    // Only process other keys if console is visible
    if (!m_visible) {
        return false;
    }
    
    switch (key) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (!m_inputText.isEmpty()) {
                executeCommand(m_inputText);
                
                // Add to command history
                m_commandHistory.append(m_inputText);
                m_historyIndex = m_commandHistory.size();
                
                // Clear input
                m_inputText.clear();
            }
            return true;
            
        case Qt::Key_Escape:
            setVisible(false);
            return true;
            
        case Qt::Key_Backspace:
            if (!m_inputText.isEmpty()) {
                m_inputText.chop(1);
            }
            return true;
            
        case Qt::Key_Up:
            // Navigate command history
            if (!m_commandHistory.isEmpty()) {
                if (m_historyIndex > 0) {
                    --m_historyIndex;
                }
                m_inputText = m_commandHistory[m_historyIndex];
            }
            return true;
            
        case Qt::Key_Down:
            // Navigate command history
            if (!m_commandHistory.isEmpty()) {
                if (m_historyIndex < m_commandHistory.size() - 1) {
                    ++m_historyIndex;
                    m_inputText = m_commandHistory[m_historyIndex];
                } else {
                    m_historyIndex = m_commandHistory.size();
                    m_inputText.clear();
                }
            }
            return true;
            
        default:
            // Add printable characters to input
            if (!text.isEmpty() && text[0].isPrint()) {
                m_inputText += text;
                return true;
            }
            break;
    }
    
    return false;
}

void DebugConsole::registerCommand(DebugCommand* command) {
    if (!command) {
        return;
    }
    
    // Add command to registry
    m_commands.insert(command->getName().toLower(), command);
    qDebug() << "Registered debug command:" << command->getName();
}

void DebugConsole::setVisible(bool visible) {
    if (m_visible != visible) {
        m_visible = visible;
        emit visibilityChanged(m_visible);
        
        qDebug() << "Debug console" << (m_visible ? "shown" : "hidden");
    }
}

DebugCommand* DebugConsole::getCommand(const QString& name) const {
    return m_commands.value(name.toLower(), nullptr);
}

void DebugConsole::executeCommand(const QString& commandText) {
    // Add command to output
    addOutput("> " + commandText);
    
    // Parse command and arguments
    QStringList parts = commandText.split(" ", Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return;
    }
    
    QString commandName = parts[0].toLower();
    QStringList args = parts.mid(1);
    
    // Handle built-in help command
    if (commandName == "help") {
        if (args.isEmpty()) {
            // General help
            addOutput("Available commands:");
            for (auto it = m_commands.begin(); it != m_commands.end(); ++it) {
                addOutput("  " + it.key() + " - " + it.value()->getDescription());
            }
            addOutput("Type 'help <command>' for specific command usage");
            return;
        } else {
            // Command-specific help
            QString helpCommand = args[0].toLower();
            DebugCommand* command = getCommand(helpCommand);
            if (command) {
                addOutput(command->getName() + " - " + command->getDescription());
                addOutput("Usage: " + command->getUsage());
            } else {
                addOutput("Unknown command: " + helpCommand);
            }
            return;
        }
    }
    
    // Handle other commands
    DebugCommand* command = getCommand(commandName);
    if (command) {
        try {
            QString result = command->execute(args, m_gameScene, m_playerController);
            if (!result.isEmpty()) {
                addOutput(result);
            }
            
            emit commandExecuted(commandName, result);
        } catch (const std::exception& e) {
            addOutput("Error executing command: " + QString(e.what()));
        }
    } else {
        addOutput("Unknown command: " + commandName);
    }
}

void DebugConsole::addOutput(const QString& text) {
    m_outputLines.append(text);
    
    // Keep only the most recent lines
    while (m_outputLines.size() > m_maxOutputLines) {
        m_outputLines.removeFirst();
    }
}

void DebugConsole::initializeGL() {
    // Initialize OpenGL functions
    initializeOpenGLFunctions();
    
    // Create shaders
    createShaders();
    
    // Create geometry
    createQuadGeometry();
}

void DebugConsole::createShaders() {
    // Create shader program
    m_consoleShader = new QOpenGLShaderProgram();
    
    // Vertex shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 position;
        
        uniform mat4 model;
        uniform mat4 projection;
        
        void main() {
            gl_Position = projection * model * vec4(position, 0.0, 1.0);
        }
    )";
    
    // Fragment shader
    const char* fragmentShaderSource = R"(
        #version 330 core
        uniform vec4 color;
        
        out vec4 fragColor;
        
        void main() {
            fragColor = color;
        }
    )";
    
    // Compile and link shaders
    if (!m_consoleShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qCritical() << "Failed to compile vertex shader:" << m_consoleShader->log();
    }
    
    if (!m_consoleShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qCritical() << "Failed to compile fragment shader:" << m_consoleShader->log();
    }
    
    if (!m_consoleShader->link()) {
        qCritical() << "Failed to link shader program:" << m_consoleShader->log();
    }
}

void DebugConsole::createQuadGeometry() {
    // Unit quad vertices (0,0) to (1,1)
    float quadVertices[] = {
        0.0f, 0.0f, // Bottom left
        1.0f, 0.0f, // Bottom right
        1.0f, 1.0f, // Top right
        0.0f, 1.0f  // Top left
    };
    
    // Create and bind VAO
    m_quadVAO.create();
    m_quadVAO.bind();
    
    // Create and bind VBO
    m_quadVBO.create();
    m_quadVBO.bind();
    
    // Upload vertex data
    m_quadVBO.allocate(quadVertices, sizeof(quadVertices));
    
    // Set up vertex attributes
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    
    // Unbind
    m_quadVBO.release();
    m_quadVAO.release();
}