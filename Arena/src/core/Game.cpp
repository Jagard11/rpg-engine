#include "core/Game.hpp"
#include "core/Window.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <thread>

Game::Game()
    : m_isRunning(false)
    , m_isInGame(false)
{
}

Game::~Game() {
    cleanup();
}

bool Game::initialize() {
    try {
        // Create window
        m_window = std::make_unique<Window>();
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

        // Initialize components
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->initialize()) {
            std::cerr << "Failed to initialize renderer" << std::endl;
            return false;
        }

        m_splashScreen = std::make_unique<SplashScreen>();
        
        // Setup splash screen callbacks
        m_splashScreen->setNewGameCallback([this](uint64_t seed) {
            createNewWorld(seed);
        });
        
        m_splashScreen->setLoadGameCallback([this](const std::string& path) {
            loadWorld(path);
        });
        
        m_splashScreen->setSaveGameCallback([this](const std::string& path) {
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

        m_splashScreen->initialize(m_window->getHandle(), this);
        
        // Register the splash screen with the window for character callbacks
        m_window->setActiveSplashScreen(m_splashScreen.get());
        
        // Ensure saves directory exists
        std::filesystem::path savesDir = "saves";
        if (!std::filesystem::exists(savesDir)) {
            std::filesystem::create_directory(savesDir);
        }
        
        // Enable depth testing
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        
        // Set clear color
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
        
        m_isRunning = true;
        std::cout << "Game initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error during initialization: " << e.what() << std::endl;
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
    
    // Render game world or splash screen
    if (m_isInGame && m_world && m_player) {
        // Render world and player view
        m_renderer->render(m_world.get(), m_player.get());
        
        // Render HUD elements
        renderHUD();
    }
    
    // Render UI elements on top
    if (m_splashScreen && m_splashScreen->isActive()) {
        m_splashScreen->render();
    }
}

void Game::renderHUD() {
    if (!m_player) return;
    
    // Debug output to confirm HUD is being called
    static bool firstHudRender = true;
    if (firstHudRender) {
        std::cout << "HUD rendering started" << std::endl;
        firstHudRender = false;
    }
    
    // Set up for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use legacy OpenGL for simpler drawing
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int width, height;
    width = m_window->getWidth();
    height = m_window->getHeight();
    glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Render crosshair
    float crosshairSize = 10.0f;
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(width/2 - crosshairSize, height/2);
    glVertex2f(width/2 + crosshairSize, height/2);
    glVertex2f(width/2, height/2 - crosshairSize);
    glVertex2f(width/2, height/2 + crosshairSize);
    glEnd();
    
    // Render jetpack indicator
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
    
    // Jetpack status indicator (red when off, white when on)
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
    
    // Render flying mode status (simple square indicators)
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
    
    // Restore OpenGL state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Game::handleInput(float deltaTime) {
    // Check for the escape key to toggle in-game menu
    if (m_isInGame && m_window->isKeyPressed(GLFW_KEY_ESCAPE) && 
        m_window->isKeyJustPressed(GLFW_KEY_ESCAPE)) {
        
        if (m_splashScreen->isActive()) {
            m_splashScreen->setInactive();
            m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            m_splashScreen->activateInGameMenu();
            m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    
    // Handle normal game input when we're playing
    if (m_isInGame && m_player && !m_splashScreen->isActive()) {
        m_player->handleInput(deltaTime, m_world.get());
        
        // Let window capture cursor for camera
        m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
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