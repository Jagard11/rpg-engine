#include "core/Game.hpp"
#include "core/Window.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <thread>
#include "debug/DebugMenu.hpp"
#include "debug/VoxelDebug.hpp"
#include "debug/DebugStats.hpp"
#include "world/VoxelManipulator.hpp"
#include <glm/gtc/constants.hpp>

// Add this global variable and function at the top of the file
static Game* g_gameInstance = nullptr;

// Add mouse button callback
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->handleMouseInput(button, action, mods);
    }
}

Game::Game()
    : m_window(nullptr)
    , m_renderer(nullptr)
    , m_world(nullptr)
    , m_player(nullptr)
    , m_splashScreen(nullptr)
    , m_debugMenu(nullptr)
    , m_voxelManipulator(nullptr)
    , m_isRunning(false)
    , m_isInGame(false)
    , m_fps(0)
    , m_debugStats(nullptr)
    , m_lastPlayerChunkPos(-1) // Initialize to an invalid position
{
    // Store the global instance for callbacks
    g_gameInstance = this;

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
        initializeDebugMenu();
        
        // Initialize debug stats
        m_debugStats = std::make_unique<Debug::DebugStats>();
        m_debugStats->initialize(m_window->getHandle(), this);

        // Initialize player
        m_player = std::make_unique<Player>();
        m_player->setPosition(glm::vec3(0.0f, 65.0f, 0.0f));

        // Initialize world with a default seed
        m_world = std::make_unique<World>(12345);
        m_world->generateChunk(glm::ivec3(0, 0, 0)); // Generate initial chunk

        // Set up mouse button callback for voxel manipulation
        glfwSetMouseButtonCallback(m_window->getHandle(), mouse_button_callback);

        // Initialize voxel manipulator
        m_voxelManipulator = std::make_unique<VoxelManipulator>();

        // Initialize debug systems
        Debug::VoxelDebug::initialize();

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
    
    // Update debug stats
    if (m_debugStats) {
        m_debugStats->update(deltaTime);
    }
    
    if (m_isInGame && m_world && m_player && !m_splashScreen->isActive()) {
        m_player->update(deltaTime, m_world.get());
        
        // Get current player chunk position
        glm::ivec3 currentPlayerChunkPos = m_world->worldToChunkPos(m_player->getPosition());
        
        // Only *evaluate* chunks needed when the player moves to a new chunk
        if (currentPlayerChunkPos != m_lastPlayerChunkPos) {
            //std::cout << "Player moved to new chunk: [" 
            //          << currentPlayerChunkPos.x << ", " << currentPlayerChunkPos.y << ", " << currentPlayerChunkPos.z
            //          << "] - Evaluating chunks needed..." << std::endl;
            m_world->evaluateChunksNeeded(m_player->getPosition());
            m_lastPlayerChunkPos = currentPlayerChunkPos; // Update last known position
        }
        
        // *Process* the load/unload queues every frame
        m_world->processChunkQueues();
        
        // Process a limited number of dirty chunk meshes per frame (ALWAYS run this)
        int meshUpdatesThisFrame = m_fps > 40 ? 3 : (m_fps > 20 ? 2 : 1);
        m_world->updateDirtyChunkMeshes(meshUpdatesThisFrame);
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
        
        // Render world and player view
        m_renderer->render(m_world.get(), m_player.get());
    }
    
    // Set up for 2D UI rendering
    glDisable(GL_DEPTH_TEST);
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
    
    // Debug stats are rendered before debug menu
    if (m_debugStats && m_isInGame) {
        m_debugStats->render();
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
    
    // Set up orthographic projection for 2D UI rendering
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1); // Top-left origin for consistency with UI
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Disable depth testing for UI
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthTestEnabled) glDisable(GL_DEPTH_TEST);
    
    // Enable blending for transparent UI elements
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (!blendEnabled) glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
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
    glVertex2f(padding, height - (padding + boxHeight));
    glVertex2f(padding + boxWidth, height - (padding + boxHeight));
    glVertex2f(padding + boxWidth, height - padding);
    glVertex2f(padding, height - padding);
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
    float centerY = height - (padding + boxHeight/2);
    
    // Main body
    glVertex2f(centerX - 10.0f, centerY + 15.0f);
    glVertex2f(centerX + 10.0f, centerY + 15.0f);
    glVertex2f(centerX, centerY - 15.0f);
    
    // Flames if enabled
    if (m_player->isJetpackEnabled()) {
        glColor4f(1.0f, 0.7f, 0.2f, 1.0f); // Orange flame
        glVertex2f(centerX - 8.0f, centerY + 15.0f);
        glVertex2f(centerX + 8.0f, centerY + 15.0f);
        glVertex2f(centerX, centerY + 25.0f);
    }
    glEnd();
    
    // Draw fuel bar
    float fuelPercentage = m_player->getJetpackFuel() / 100.0f;
    float barWidth = 60.0f;
    float barHeight = 10.0f;
    float barX = padding + indicatorSize + 20.0f;
    float barY = height - (padding + (boxHeight + barHeight) / 2);
    
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
    float statusY = height - (padding + boxHeight + 15.0f);
    
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
    
    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Restore OpenGL state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (!blendEnabled) glDisable(GL_BLEND);
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
    
    // Check for F9 key to toggle debug stats
    if (m_debugStats && m_window->isKeyJustPressed(GLFW_KEY_F9)) {
        m_debugStats->toggleVisibility();
        std::cout << "Debug stats " << (m_debugStats->isVisible() ? "enabled" : "disabled") << std::endl;
    }
    
    // Check for F12 key to dump debug information
    if (m_window->isKeyJustPressed(GLFW_KEY_F12)) {
        std::cout << "F12 pressed - Dumping debug information..." << std::endl;
        Debug::VoxelDebug::dumpDebugInfo(m_world.get(), m_player.get());
    }
    
    // Forward keys to debug menu if it's active
    if (m_debugMenu && m_debugMenu->isActive()) {
        // Character input will be handled through the window's character callback
        
        // Handle special keys
        for (int key : {GLFW_KEY_ENTER, GLFW_KEY_BACKSPACE, GLFW_KEY_ESCAPE, 
                  GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_TAB}) {
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
        }
    }
}

void Game::createNewWorld(uint64_t seed) {
    std::cout << "Creating new world with seed: " << seed << std::endl;
    
    m_world = std::make_unique<World>(seed);
    m_world->initialize();
    
    // Create player at a default spawn position
    m_player = std::make_unique<Player>();
    
    // Set initial player position - significantly higher to ensure the player doesn't get stuck
    m_player->setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
    
    // Initialize voxel manipulator with the world
    if (m_voxelManipulator) {
        m_voxelManipulator->initialize(m_world.get());
    }
    
    m_isInGame = true;
    
    // Lock cursor for game mode
    m_window->setInputMode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set up world debugging
    if (m_debugMenu) {
        // Register debug commands for the world if needed in the future
    }
    
    // Force an initial render to ensure chunks are visible
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Ensure renderer has initialized its buffers
    if (m_renderer) {
        if (!m_renderer->isBuffersInitialized()) {
            std::cout << "Initializing renderer buffers..." << std::endl;
            m_renderer->setupBuffers();
        }
        
        // Force initial rendering of the world
        std::cout << "Performing initial world render..." << std::endl;
        m_renderer->render(m_world.get(), m_player.get());
        m_window->swapBuffers();
    }
    
    // Set player pointer in renderer
    if (m_renderer && m_player) {
        m_renderer->setPlayer(m_player.get());
    }
    
    std::cout << "World creation complete" << std::endl;
}

bool Game::loadWorld(const std::string& savePath) {
    // Ensure the save directory exists
    std::filesystem::create_directories(std::filesystem::path(savePath).parent_path());
    
    // Create a new world with seed 0 (will be overwritten by load)
    m_world = std::make_unique<World>(0);
    
    // Try to load world data
    if (!m_world->deserialize(savePath)) {
        std::cerr << "Failed to load world: " << savePath << std::endl;
        return false;
    }
    
    // Load player data if it exists
    std::string playerSavePath = savePath + ".player";
    if (std::filesystem::exists(playerSavePath)) {
        m_player = std::make_unique<Player>();
        m_player->loadFromFile(playerSavePath);
    } else {
        // Create a new player at a reasonable position
        m_player = std::make_unique<Player>();
        m_player->setPosition(glm::vec3(0.0f, 100.0f, 0.0f));
    }
    
    // Initialize chunks around player
    if (m_world && m_player) {
        m_world->evaluateChunksNeeded(m_player->getPosition());
    }
    
    return true;
}

void Game::saveWorld(const std::string& savePath) {
    if (!m_world || !m_player) {
        std::cerr << "Cannot save world: world or player is null" << std::endl;
        return;
    }
    
    // Ensure the save directory exists
    std::filesystem::create_directories(std::filesystem::path(savePath).parent_path());
    
    // Save world data
    m_world->serialize(savePath);
    m_player->saveToFile(savePath + ".player");
    
    std::cout << "World saved to: " << savePath << std::endl;
}

void Game::cleanup() {
    if (m_world && m_player && m_isInGame) {
        saveWorld("saves/autosave.sav");
    }
}

void Game::initializeDebugMenu() {
    // Create debug menu
    m_debugMenu = std::make_unique<Debug::DebugMenu>();
    m_debugMenu->initialize(m_window->getHandle(), this);
    
    // Register command to toggle flying mode
    m_debugMenu->registerCommand("fly", "Toggle flying mode (no gravity)",
        [this](const std::vector<std::string>& args) {
            if (m_player) {
                m_player->toggleFlying();
                m_debugMenu->commandOutput("Flying mode " + 
                    std::string(m_player->isFlying() ? "enabled" : "disabled"));
            } else {
                m_debugMenu->commandOutput("No player exists!");
            }
        });
    
    // Register command to toggle collision
    m_debugMenu->registerCommand("noclip", "Toggle collision detection",
        [this](const std::vector<std::string>& args) {
            if (m_player) {
                bool currentCollision = m_player->hasCollision();
                m_player->setCollision(!currentCollision);
                m_debugMenu->commandOutput("Collision " + 
                    std::string(!currentCollision ? "enabled" : "disabled"));
            } else {
                m_debugMenu->commandOutput("No player exists!");
            }
        });
        
    // Register command to toggle greedy meshing
    m_debugMenu->registerCommand("greedy", "Toggle greedy meshing algorithm",
        [this](const std::vector<std::string>& args) {
            if (m_world) {
                bool currentState = m_world->isGreedyMeshingEnabled();
                m_world->setGreedyMeshingEnabled(!currentState);
                m_debugMenu->commandOutput("Greedy meshing " + 
                    std::string(!currentState ? "enabled" : "disabled"));
            } else {
                m_debugMenu->commandOutput("No world exists!");
            }
        });
        
    // Register command to change view distance
    m_debugMenu->registerCommand("viewdist", "Set view distance in chunks (e.g., 'viewdist 8')",
        [this](const std::vector<std::string>& args) {
            if (args.size() < 1) {
                m_debugMenu->commandOutput("Usage: viewdist <distance>");
                return;
            }
            
            try {
                int distance = std::stoi(args[0]);
                if (distance < 1) distance = 1;
                if (distance > 16) distance = 16; // Cap at 16 chunks for performance
                
                if (m_world) {
                    m_world->setViewDistance(distance);
                    m_debugMenu->commandOutput("View distance set to " + std::to_string(distance) + " chunks");
                } else {
                    m_debugMenu->commandOutput("No world exists!");
                }
            } catch (const std::exception& e) {
                m_debugMenu->commandOutput("Invalid distance. Usage: viewdist <distance>");
            }
        });
        
    // Register command to get current position
    m_debugMenu->registerCommand("pos", "Display current player position",
        [this](const std::vector<std::string>& args) {
            if (m_player) {
                const glm::vec3& pos = m_player->getPosition();
                std::stringstream ss;
                ss << "Position: X=" << pos.x << ", Y=" << pos.y << ", Z=" << pos.z;
                m_debugMenu->commandOutput(ss.str());
            } else {
                m_debugMenu->commandOutput("No player exists!");
            }
        });
        
    // Register command to toggle stats display
    m_debugMenu->registerCommand("stats", "Toggle debug statistics display",
        [this](const std::vector<std::string>& args) {
            if (m_debugStats) {
                m_debugStats->toggleVisibility();
                m_debugMenu->commandOutput("Debug stats " + 
                    std::string(m_debugStats->isVisible() ? "enabled" : "disabled"));
            } else {
                m_debugMenu->commandOutput("Debug stats not available!");
            }
        });
        
    // Register command to teleport
    m_debugMenu->registerCommand("tp", "Teleport to coordinates (e.g., 'tp 0 100 0')",
        [this](const std::vector<std::string>& args) {
            if (args.size() < 3) {
                m_debugMenu->commandOutput("Usage: tp <x> <y> <z>");
                return;
            }
            
            try {
                float x = std::stof(args[0]);
                float y = std::stof(args[1]);
                float z = std::stof(args[2]);
                
                if (m_player) {
                    m_player->setPosition(glm::vec3(x, y, z));
                    std::stringstream ss;
                    ss << "Teleported to X=" << x << ", Y=" << y << ", Z=" << z;
                    m_debugMenu->commandOutput(ss.str());
                } else {
                    m_debugMenu->commandOutput("No player exists!");
                }
            } catch (const std::exception& e) {
                m_debugMenu->commandOutput("Invalid coordinates. Usage: tp <x> <y> <z>");
            }
        });

    // Pass the debug menu to the renderer if needed
    if (m_renderer) {
        m_renderer->setDebugMenu(m_debugMenu.get());
    }

    // Set the debug menu as active in the window for character input
    m_window->setActiveDebugMenu(m_debugMenu.get());
}

// Add new method to handle mouse input
void Game::handleMouseInput(int button, int action, int mods) {
    // Skip if not in game
    if (!m_isInGame || !m_world || !m_player) {
        return;
    }
    
    // Skip if splash screen is active or debug menu is open
    if ((m_splashScreen && m_splashScreen->isActive()) || 
        (m_debugMenu && m_debugMenu->isActive())) {
        return;
    }
    
    // Process mouse input for voxel manipulation
    if (m_voxelManipulator) {
        m_voxelManipulator->processInput(
            m_world.get(), 
            m_player.get(), 
            button, 
            action == GLFW_PRESS,
            m_renderer.get() // Pass the renderer to access highlight information
        );
    }
} 