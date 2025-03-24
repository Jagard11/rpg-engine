// src/arena/debug/debug_system.cpp
// src/arena/debug/debug_system.h
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
      m_playerController(player)
{
    // Create debug components
    m_console = std::make_unique<DebugConsole>(scene, player, this);
    m_frustumVisualizer = std::make_unique<FrustumVisualizer>(this);
    
    // Create commands
    m_locationCommand = std::make_unique<LocationCommand>(this);
    m_teleportCommand = std::make_unique<TeleportCommand>(this);
    m_frustumCullCommand = std::make_unique<FrustumCullCommand>(m_frustumVisualizer.get(), this);
    
    qDebug() << "Debug system created";
}

DebugSystem::~DebugSystem()
{
    qDebug() << "Debug system destroyed";
}

void DebugSystem::initialize()
{
    // Initialize console
    m_console->initialize();
    
    // Initialize visualizers
    m_frustumVisualizer->initialize();
    
    // Register commands
    registerCommands();
    
    qDebug() << "Debug system initialized";
}

void DebugSystem::render(const QMatrix4x4& viewMatrix, const QMatrix4x4& projectionMatrix,
                        int screenWidth, int screenHeight)
{
    // Render frustum visualizer
    m_frustumVisualizer->render(viewMatrix, projectionMatrix);
    
    // Render console (always last to overlay everything)
    m_console->render(screenWidth, screenHeight);
}

bool DebugSystem::handleKeyPress(int key, const QString& text)
{
    // First, let the console handle the input
    return m_console->handleKeyPress(key, text);
}

bool DebugSystem::isConsoleVisible() const
{
    if (!m_console) {
        return false;
    }
    
    return m_console->isVisible();
}

void DebugSystem::toggleConsoleVisibility()
{
    if (!m_console) {
        return;
    }
    
    m_console->setVisible(!m_console->isVisible());
}

void DebugSystem::toggleFrustumVisualization()
{
    if (!m_frustumVisualizer) {
        return;
    }
    
    m_frustumVisualizer->setEnabled(!m_frustumVisualizer->isEnabled());
}

void DebugSystem::setConsoleWidget(const QVariant& widget)
{
    if (!m_console) {
        return;
    }
    
    try {
        // Set the render widget property for the console
        m_console->setProperty("render_widget", widget);
    } catch (const std::exception& e) {
        qWarning() << "Exception setting console widget:" << e.what();
    } catch (...) {
        qWarning() << "Unknown exception setting console widget";
    }
}

void DebugSystem::registerCommands()
{
    // Register all commands with the console
    m_console->registerCommand(m_locationCommand.get());
    m_console->registerCommand(m_teleportCommand.get());
    m_console->registerCommand(m_frustumCullCommand.get());
    
    qDebug() << "Debug commands registered";
}