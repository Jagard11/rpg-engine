// src/arena/debug/debug_system.cpp
#include "../../../include/arena/debug/debug_system.h"
#include "../../../include/arena/debug/console/debug_console.h"
#include "../../../include/arena/debug/commands/location_command.h"
#include "../../../include/arena/debug/commands/teleport_command.h"
#include "../../../include/arena/debug/commands/frustum_cull_command.h"
#include "../../../include/arena/debug/visualizers/frustum_visualizer.h"
#include "../../../include/arena/game/player_controller.h"
#include "../../../include/arena/core/arena_core.h"
#include <QApplication>
#include <QDebug>

DebugSystem::DebugSystem(GameScene* scene, PlayerController* player, QObject* parent)
    : QObject(parent),
      m_gameScene(scene),
      m_playerController(player),
      m_console(nullptr),
      m_frustumVisualizer(nullptr),
      m_locationCommand(nullptr),
      m_teleportCommand(nullptr),
      m_frustumCullCommand(nullptr)
{
    // Verify inputs
    if (!scene || !player) {
        qWarning() << "Debug system created with null scene or player";
    }
    
    try {
        // Create console
        qDebug() << "Creating debug console...";
        m_console = std::make_unique<DebugConsole>(scene, player, this);
        
        // Create visualizers
        qDebug() << "Creating frustum visualizer...";
        m_frustumVisualizer = std::make_unique<FrustumVisualizer>(this);
        
        // Create commands
        qDebug() << "Creating debug commands...";
        m_locationCommand = std::make_unique<LocationCommand>(this);
        m_teleportCommand = std::make_unique<TeleportCommand>(this);
        
        qDebug() << "Debug system objects created successfully";
        
        // Note: We'll create the FrustumCullCommand later during initialization
        // as it depends on the visualizer
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in DebugSystem constructor:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in DebugSystem constructor";
    }
    
    qDebug() << "Debug system created";
}

DebugSystem::~DebugSystem()
{
    qDebug() << "Debug system destroyed";
}

void DebugSystem::initialize()
{
    qDebug() << "Initializing debug system...";
    
    // Create the frustum cull command here since it needs the visualizer
    try {
        if (m_frustumVisualizer) {
            m_frustumCullCommand = std::make_unique<FrustumCullCommand>(m_frustumVisualizer.get(), this);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "Exception creating frustum cull command:" << e.what();
    }
    
    // Initialize components
    try {
        // Initialize console if available
        if (m_console) {
            qDebug() << "Initializing debug console...";
            m_console->initialize();
        }
        
        // Initialize visualizers if available
        // This will be initialized using the current OpenGL context if available
        if (m_frustumVisualizer && QOpenGLContext::currentContext()) {
            qDebug() << "Initializing frustum visualizer...";
            m_frustumVisualizer->initialize();
        }
        
        // Register commands after console is initialized
        if (m_console) {
            qDebug() << "Registering debug commands...";
            registerCommands();
        }
        
        qDebug() << "Debug system initialization complete";
    }
    catch (const std::exception& e) {
        qWarning() << "Exception in debug system initialization:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception in debug system initialization";
    }
}

void DebugSystem::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix,
                        int screenWidth, int screenHeight)
{
    // Render frustum visualizer if enabled
    if (m_frustumVisualizer && m_frustumVisualizer->isEnabled()) {
        try {
            // Pass view and projection matrices to the visualizer
            m_frustumVisualizer->render(viewMatrix, projectionMatrix);
            
            // Log that we're rendering the frustum visualizer
            static int logCounter = 0;
            if (logCounter++ % 100 == 0) { // Log only periodically to avoid spam
                qDebug() << "Rendering frustum visualizer (enabled state:" 
                        << m_frustumVisualizer->isEnabled() << ")";
            }
        }
        catch (const std::exception& e) {
            qWarning() << "Exception rendering frustum visualizer:" << e.what();
        }
        catch (...) {
            qWarning() << "Unknown exception rendering frustum visualizer";
        }
    }
    
    // Render console if visible
    if (m_console && m_console->isVisible()) {
        try {
            m_console->render(screenWidth, screenHeight);
        }
        catch (const std::exception& e) {
            qWarning() << "Exception rendering debug console:" << e.what();
        }
        catch (...) {
            qWarning() << "Unknown exception rendering debug console";
        }
    }
}

bool DebugSystem::handleKeyPress(int key, const QString& text)
{
    // If console is available, let it handle the input
    if (m_console) {
        try {
            return m_console->handleKeyPress(key, text);
        }
        catch (const std::exception& e) {
            qWarning() << "Exception handling key press in debug console:" << e.what();
        }
        catch (...) {
            qWarning() << "Unknown exception handling key press in debug console";
        }
    }
    
    return false;
}

bool DebugSystem::isConsoleVisible() const
{
    if (!m_console) {
        return false;
    }
    
    try {
        return m_console->isVisible();
    }
    catch (const std::exception& e) {
        qWarning() << "Exception checking console visibility:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception checking console visibility";
    }
    
    return false;
}

void DebugSystem::toggleConsoleVisibility()
{
    if (!m_console) {
        return;
    }
    
    try {
        m_console->setVisible(!m_console->isVisible());
    }
    catch (const std::exception& e) {
        qWarning() << "Exception toggling console visibility:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception toggling console visibility";
    }
}

void DebugSystem::toggleFrustumVisualization()
{
    if (!m_frustumVisualizer) {
        qWarning() << "Cannot toggle frustum visualization: visualizer not available";
        return;
    }
    
    try {
        bool newState = !m_frustumVisualizer->isEnabled();
        m_frustumVisualizer->setEnabled(newState);
        qDebug() << "Frustum visualization toggled to: " << newState;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception toggling frustum visualization:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception toggling frustum visualization";
    }
}

void DebugSystem::setConsoleWidget(const QVariant& widget)
{
    if (!m_console) {
        qWarning() << "Cannot set console widget: console not available";
        return;
    }
    
    try {
        // Set the render widget property for the console
        m_console->setProperty("render_widget", widget);
    }
    catch (const std::exception& e) {
        qWarning() << "Exception setting console widget:" << e.what();
    }
    catch (...) {
        qWarning() << "Unknown exception setting console widget";
    }
}

void DebugSystem::registerCommands()
{
    // Verify console is available
    if (!m_console) {
        qWarning() << "Cannot register commands: console not available";
        return;
    }
    
    // Register location command
    if (m_locationCommand) {
        try {
            m_console->registerCommand(m_locationCommand.get());
        }
        catch (const std::exception& e) {
            qWarning() << "Exception registering location command:" << e.what();
        }
    }
    
    // Register teleport command
    if (m_teleportCommand) {
        try {
            m_console->registerCommand(m_teleportCommand.get());
        }
        catch (const std::exception& e) {
            qWarning() << "Exception registering teleport command:" << e.what();
        }
    }
    
    // Register frustum cull command
    if (m_frustumCullCommand) {
        try {
            m_console->registerCommand(m_frustumCullCommand.get());
            qDebug() << "Frustum cull command registered successfully";
        }
        catch (const std::exception& e) {
            qWarning() << "Exception registering frustum cull command:" << e.what();
        }
    }
    
    qDebug() << "Debug commands registered";
}