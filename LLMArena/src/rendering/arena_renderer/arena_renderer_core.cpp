// src/rendering/arena_renderer/arena_renderer_core.cpp
#include "../../include/rendering/arena_renderer.h"
#include "../../include/game/game_scene.h"
#include "../../include/game/player_controller.h"
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebChannel>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QDebug>

ArenaRenderer::ArenaRenderer(QWidget *parent, CharacterManager *charManager) 
    : QObject(parent), initialized(false), characterManager(charManager) {
    
    // Check if WebGL is supported before initializing
    if (!isWebGLSupported()) {
        qWarning() << "WebGL is not supported on this system";
        throw std::runtime_error("WebGL not supported");
    }
    
    // Initialize web view for WebGL rendering
    webView = new QWebEngineView(qobject_cast<QWidget*>(parent));
    
    // Enable WebGL and hardware acceleration
    webView->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    webView->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    
    // Enable developer tools for debugging if needed
    webView->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    
    // Enable hardware rendering - no longer forcing software rendering
    webView->page()->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    
    // Set additional WebEngine settings to prefer hardware acceleration
    webView->page()->profile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    
    // Connect signals for page loading
    connect(webView, &QWebEngineView::loadFinished, this, &ArenaRenderer::handleLoadFinished);
    
    // Create the game scene
    gameScene = new GameScene(this);
    
    // Create the player controller
    playerController = new PlayerController(gameScene, this);
    
    // Connect player controller signals using lambda to convert QVector3D to separate coordinates
    connect(playerController, &PlayerController::positionChanged, 
            [this](const QVector3D &pos) {
                // Convert QVector3D to separate x,y,z coordinates
                this->updatePlayerPosition(pos.x(), pos.y(), pos.z());
            });
    
    connect(playerController, &PlayerController::rotationChanged, 
            [this](float rotation) {
                // Update player position (which includes rotation update)
                QVector3D pos = playerController->getPosition();
                updatePlayerPosition(pos.x(), pos.y(), pos.z());
            });
    
    // Set up the web channel for JavaScript communication
    webChannel = new QWebChannel(this);
    webChannel->registerObject("arenaRenderer", this);
    webView->page()->setWebChannel(webChannel);
}

ArenaRenderer::~ArenaRenderer() {
    // Clean up resources
    delete webView;
}

void ArenaRenderer::initialize() {
    // Load the HTML file for WebGL rendering
    QString htmlPath = QDir::currentPath() + "/resources/arena.html";
    QFile htmlFile(htmlPath);
    
    if (htmlFile.exists()) {
        qDebug() << "Loading existing arena HTML file from:" << htmlPath;
        webView->load(QUrl::fromLocalFile(htmlPath));
    } else {
        qDebug() << "Creating new arena HTML file at:" << htmlPath;
        
        // Create resources directory if it doesn't exist
        QDir dir;
        dir.mkpath(QDir::currentPath() + "/resources");
        
        // Create HTML file with WebGL setup
        if (createArenaHtmlFile(htmlPath)) {
            // Load the created file
            webView->load(QUrl::fromLocalFile(htmlPath));
        } else {
            qWarning() << "Failed to create HTML file at:" << htmlPath;
            throw std::runtime_error("Failed to create WebGL arena HTML file");
        }
    }
}

void ArenaRenderer::handleLoadFinished(bool ok) {
    if (ok) {
        qDebug() << "WebGL page loaded successfully";
        
        // Run diagnostic script to get WebGL details
        webView->page()->runJavaScript(
            "function getWebGLInfo() {"
            "  try {"
            "    const canvas = document.createElement('canvas');"
            "    const gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');"
            "    if (!gl) return 'WebGL not supported';"
            "    const debugInfo = gl.getExtension('WEBGL_debug_renderer_info');"
            "    let vendor = gl.getParameter(gl.VENDOR);"
            "    let renderer = gl.getParameter(gl.RENDERER);"
            "    if (debugInfo) {"
            "      vendor = gl.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);"
            "      renderer = gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);"
            "    }"
            "    return 'WebGL: ' + vendor + ' - ' + renderer;"
            "  } catch(e) {"
            "    return 'Error getting WebGL info: ' + e.message;"
            "  }"
            "}"
            "getWebGLInfo();",
            [this](const QVariant &result) {
                qDebug() << "WebGL Info:" << result.toString();
            }
        );
        
        // Initialize WebGL context
        initializeWebGL();
        
        // Set arena parameters
        setArenaParameters(10.0, 2.0);
        
        // Create the player entity
        playerController->createPlayerEntity();
        
        // Start player controller updates
        playerController->startUpdates();
        
        initialized = true;
        emit renderingInitialized();
    } else {
        qDebug() << "Failed to load WebGL page";
        
        // Try to notify the user about the error
        emit renderingInitialized(); // Still emit signal so UI updates
    }
}

void ArenaRenderer::setActiveCharacter(const QString &name) {
    activeCharacter = name;
}

void ArenaRenderer::createArena(double radius, double wallHeight) {
    setArenaParameters(radius, wallHeight);
}

GameScene* ArenaRenderer::getGameScene() const {
    return gameScene;
}

PlayerController* ArenaRenderer::getPlayerController() const {
    return playerController;
}

void ArenaRenderer::handleJavaScriptMessage(const QString &message) {
    qDebug() << "JavaScript message: " << message;
}