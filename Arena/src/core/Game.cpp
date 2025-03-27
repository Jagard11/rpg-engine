#include "core/Game.hpp"
#include "core/Window.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <thread>
#include "debug/DebugMenu.hpp"

Game::Game()
    : m_window(nullptr)
    , m_renderer(nullptr)
    , m_world(nullptr)
    , m_player(nullptr)
    , m_splashScreen(nullptr)
    , m_debugMenu(nullptr)
    , m_isRunning(false)
    , m_isInGame(false)
    , m_fps(0)
{
    if (!initialize()) {
        throw std::runtime_error("Failed to initialize game");
    }
}

Game::~Game() {
    cleanup();
}

bool Game::initialize() {
    try {
        // Create window
        m_window = std::make_unique<Window>(1280, 720, "Voxel Game");
        if (!m_window->initialize()) {
            std::cerr << "Failed to initialize window" << std::endl;
            return false;
        }

        // Initialize GLEW
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::cerr << "GLEW initialization failed: " << glewGetErrorString(err) << std::endl;
            return false;
        }

        // Initialize renderer
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->initialize()) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }

        // Initialize splash screen
        m_splashScreen = std::make_unique<SplashScreen>();
        m_splashScreen->initialize(m_window->getHandle(), this);
        m_window->setActiveSplashScreen(m_splashScreen.get());
        m_renderer->setSplashScreen(m_splashScreen.get());

        // Set up splash screen callbacks
        m_splashScreen->setNewGameCallback([this](long seed) {
            std::cout << "Creating new world with seed: " << seed << std::endl;
            createNewWorld(seed);
            m_isInGame = true;
        });

        m_splashScreen->setLoadGameCallback([this](const std::string& path) {
            std::cout << "Loading world from: " << path << std::endl;
            if (loadWorld(path)) {
                m_isInGame = true;
            }
        });

        m_splashScreen->setSaveGameCallback([this](const std::string& path) {
            std::cout << "Saving world to: " << path << std::endl;
            saveWorld(path);
        });

        m_splashScreen->setQuitCallback([this](bool quitToDesktop) {
            if (quitToDesktop) {
                m_isRunning = false;
            } else {
                m_isInGame = false;
                m_world.reset();
                m_player.reset();
                m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        });

        // Initialize debug menu
        m_debugMenu = std::make_unique<Debug::DebugMenu>();
        m_debugMenu->initialize(m_window->getHandle(), this);
        m_window->setActiveDebugMenu(m_debugMenu.get());
        m_renderer->setDebugMenu(m_debugMenu.get());

        // Initialize player
        m_player = std::make_unique<Player>();
        m_player->setPosition(glm::vec3(0.0f, 65.0f, 0.0f));

        // Initialize world with a default seed
        m_world = std::make_unique<World>(12345);
        m_world->generateChunk(glm::ivec3(0, 0, 0)); // Generate initial chunk

        m_isRunning = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception during initialization: " << e.what() << std::endl;
        return false;
    }
}

void Game::run() {
    auto lastTime = std::chrono::high_resolution_clock::now();
    int frameCount = 0;
    float frameTimer = 0.0f;
    
    while (m_isRunning && !m_window->shouldClose()) {
        // Calculate delta time
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Update FPS counter
        frameCount++;
        frameTimer += deltaTime;
        if (frameTimer >= 1.0f) {
            m_fps = frameCount;
            frameCount = 0;
            frameTimer -= 1.0f;
            std::cout << "FPS: " << m_fps << std::endl;
        }

        // Process input
        m_window->pollEvents();
        handleInput(deltaTime);

        // Update game state
        update(deltaTime);

        // Render
        render();
        
        // Swap buffers
        m_window->swapBuffers();
        
        // Small sleep to prevent 100% CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Game::update(float deltaTime) {
    // Update debug menu
    if (m_debugMenu) {
        m_debugMenu->update(deltaTime);
    }
    
    if (m_isInGame && m_world && m_player && !m_splashScreen->isActive()) {
        m_player->update(deltaTime, m_world.get());
        m_world->updateChunks(m_player->getPosition());
    }
}

void Game::render() {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Ensure renderer is valid
    if (!m_renderer) {
        std::cerr << "Renderer is not initialized" << std::endl;
        return;
    }
    
    // Save OpenGL state
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    
    // Game world rendering - This should be first (3D content)
    if (m_isInGame && m_world && m_player) {
        // Set up for 3D rendering
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        
        // Render world and player view
        m_renderer->render(m_world.get(), m_player.get());
    }
    
    // Set up for 2D UI rendering
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render HUD elements
    if (m_isInGame && m_player) {
        renderHUD();
    }
    
    // UI elements come next
    if (m_splashScreen && m_splashScreen->isActive()) {
        m_splashScreen->render();
    }
    
    // Debug menu is rendered last
    if (m_debugMenu && m_debugMenu->isActive()) {
        m_debugMenu->render();
    }
    
    // Restore OpenGL state
    glUseProgram(currentProgram);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);
    glBindVertexArray(currentVAO);
}

void Game::renderHUD() {
    if (!m_player) return;
    
    // Get window dimensions
    int width = m_window->getWidth();
    int height = m_window->getHeight();
    
    // Render crosshair at screen center
    float crosshairSize = 10.0f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(width/2 - crosshairSize, height/2);
    glVertex2f(width/2 + crosshairSize, height/2);
    glVertex2f(width/2, height/2 - crosshairSize);
    glVertex2f(width/2, height/2 + crosshairSize);
    glEnd();
    
    // Render jetpack indicator in bottom left corner
    float indicatorSize = 30.0f;
    float padding = 20.0f;
    float boxWidth = 120.0f;
    float boxHeight = 40.0f;
    
    // Background box
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(padding, padding);
    glVertex2f(padding + boxWidth, padding);
    glVertex2f(padding + boxWidth, padding + boxHeight);
    glVertex2f(padding, padding + boxHeight);
    glEnd();
    
    // Jetpack status indicator
    if (m_player->isJetpackEnabled()) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // White when on
    } else {
        glColor4f(1.0f, 0.2f, 0.2f, 1.0f); // Red when off
    }
    
    glBegin(GL_TRIANGLES);
    // Draw a jetpack icon
    float centerX = padding + indicatorSize/2 + 10.0f;
    float centerY = padding + boxHeight/2;
    
    // Main body
    glVertex2f(centerX - 10.0f, centerY - 15.0f);
    glVertex2f(centerX + 10.0f, centerY - 15.0f);
    glVertex2f(centerX, centerY + 15.0f);
    
    // Flames if enabled
    if (m_player->isJetpackEnabled()) {
        glColor4f(1.0f, 0.7f, 0.2f, 1.0f); // Orange flame
        glVertex2f(centerX - 8.0f, centerY - 15.0f);
        glVertex2f(centerX + 8.0f, centerY - 15.0f);
        glVertex2f(centerX, centerY - 25.0f);
    }
    glEnd();
    
    // Draw fuel bar
    float fuelPercentage = m_player->getJetpackFuel() / 100.0f;
    float barWidth = 60.0f;
    float barHeight = 10.0f;
    float barX = padding + indicatorSize + 20.0f;
    float barY = padding + (boxHeight - barHeight) / 2;
    
    // Background
    glColor4f(0.3f, 0.3f, 0.3f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(barX, barY);
    glVertex2f(barX + barWidth, barY);
    glVertex2f(barX + barWidth, barY + barHeight);
    glVertex2f(barX, barY + barHeight);
    glEnd();
    
    // Fuel level - color changes based on fuel level
    if (fuelPercentage > 0.6f) {
        glColor4f(0.0f, 1.0f, 0.0f, 0.8f); // Green when high
    } else if (fuelPercentage > 0.3f) {
        glColor4f(1.0f, 1.0f, 0.0f, 0.8f); // Yellow when medium
    } else {
        glColor4f(1.0f, 0.0f, 0.0f, 0.8f); // Red when low
    }
    
    glBegin(GL_QUADS);
    glVertex2f(barX, barY);
    glVertex2f(barX + barWidth * fuelPercentage, barY);
    glVertex2f(barX + barWidth * fuelPercentage, barY + barHeight);
    glVertex2f(barX, barY + barHeight);
    glEnd();
    
    // Render flying mode status
    float statusSize = 10.0f;
    float statusY = padding + boxHeight + 15.0f;
    
    // Draw flying status indicator
    if (m_player->isFlying()) {
        glColor4f(0.2f, 0.6f, 1.0f, 1.0f); // Blue when flying
    } else {
        glColor4f(0.6f, 0.4f, 0.2f, 1.0f); // Brown when walking
    }
    
    glBegin(GL_QUADS);
    glVertex2f(padding, statusY);
    glVertex2f(padding + statusSize, statusY);
    glVertex2f(padding + statusSize, statusY + statusSize);
    glVertex2f(padding, statusY + statusSize);
    glEnd();
}

void Game::handleInput(float deltaTime) {
    // Check for F8 key to toggle debug menu
    if (m_debugMenu && m_window->isKeyJustPressed(GLFW_KEY_F8)) {
        // Call handleKeyPress on the debug menu to toggle it
        m_debugMenu->handleKeyPress(GLFW_KEY_F8, GLFW_PRESS);
        
        // If the player exists and we're toggling the debug menu off, prevent camera jump
        if (m_player && !m_debugMenu->isActive()) {
            m_player->ignoreNextMouseMovement();
        }
    }
    
    // Forward keys to debug menu if it's active
    if (m_debugMenu && m_debugMenu->isActive()) {
        // Character input will be handled through the window's character callback
        
        // Handle special keys
        for (int key : {GLFW_KEY_ENTER, GLFW_KEY_BACKSPACE, GLFW_KEY_ESCAPE, 
                       GLFW_KEY_UP, GLFW_KEY_DOWN}) {
            if (m_window->isKeyJustPressed(key)) {
                if (m_debugMenu->handleKeyPress(key, GLFW_PRESS)) {
                    return; // Key was handled by debug menu
                }
            }
        }
    }
    
    // Check for the escape key to toggle in-game menu, but only if debug menu is not active
    if (m_isInGame && m_window->isKeyPressed(GLFW_KEY_ESCAPE) && 
        m_window->isKeyJustPressed(GLFW_KEY_ESCAPE) && 
        (!m_debugMenu || !m_debugMenu->isActive())) {
        
        if (m_splashScreen->isActive()) {
            m_splashScreen->setInactive();
            m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            m_splashScreen->activateInGameMenu();
            m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    
    // Handle normal game input when we're playing and debug menu is not active
    if (m_isInGame && m_player && !m_splashScreen->isActive() && 
        (!m_debugMenu || !m_debugMenu->isActive())) {
        
        m_player->handleInput(deltaTime, m_world.get());
        
        // Let window capture cursor for camera
        m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else if (!m_debugMenu || !m_debugMenu->isActive()) {
        // When in menus, let the cursor be visible
        m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
        // Forward keypresses to the splash screen
        if (m_splashScreen) {
            // Handle key presses in the splash screen
            for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_Z; key++) {
                if (m_window->isKeyJustPressed(key)) {
                    m_splashScreen->handleInput(key, GLFW_PRESS);
                }
            }
            
            // Also handle backspace and enter
            if (m_window->isKeyJustPressed(GLFW_KEY_BACKSPACE)) {
                m_splashScreen->handleInput(GLFW_KEY_BACKSPACE, GLFW_PRESS);
            }
            if (m_window->isKeyJustPressed(GLFW_KEY_ENTER)) {
                m_splashScreen->handleInput(GLFW_KEY_ENTER, GLFW_PRESS);
            }
        }
    }
}

void Game::createNewWorld(uint64_t seed) {
    std::cout << "Creating new world with seed: " << seed << std::endl;
    m_world = std::make_unique<World>(seed);
    m_world->initialize();
    
    m_player = std::make_unique<Player>();
    m_isInGame = true;
    
    // Set initial player position - significantly higher to ensure the player doesn't get stuck
    m_player->setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
    
    // Lock cursor for game mode
    m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

bool Game::loadWorld(const std::string& savePath) {
    try {
        std::cout << "Loading world from: " << savePath << std::endl;
        m_world = std::make_unique<World>(0); // Seed will be overwritten by load
        if (!m_world->loadFromFile(savePath)) {
            std::cerr << "Failed to load world from file: " << savePath << std::endl;
            return false;
        }
        
        m_player = std::make_unique<Player>();
        
        // Load player data too if available
        std::string playerSavePath = savePath + ".player";
        if (std::filesystem::exists(playerSavePath)) {
            m_player->loadFromFile(playerSavePath);
        }
        
        m_isInGame = true;
        
        // Lock cursor for game mode
        m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception while loading world: " << e.what() << std::endl;
        return false;
    }
}

void Game::saveWorld(const std::string& savePath) {
    if (m_world && m_player) {
        try {
            std::cout << "Saving world to: " << savePath << std::endl;
            
            // Create saves directory if it doesn't exist
            std::filesystem::path saveDir = std::filesystem::path(savePath).parent_path();
            if (!std::filesystem::exists(saveDir)) {
                std::filesystem::create_directories(saveDir);
            }
            
            // Save world and player data
            m_world->saveToFile(savePath);
            m_player->saveToFile(savePath + ".player");
        }
        catch (const std::exception& e) {
            std::cerr << "Error saving world: " << e.what() << std::endl;
        }
    }
}

void Game::cleanup() {
    if (m_world && m_player && m_isInGame) {
        saveWorld("saves/autosave.sav");
    }
} 