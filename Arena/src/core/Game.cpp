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

// Add this global variable and function at the top of the file
static Game* g_gameInstance = nullptr;

// Add mouse button callback
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (g_gameInstance) {
        g_gameInstance->handleMouseInput(button, action, mods);
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
        
        // Reinitialize renderer buffers to fix the "Cannot render world - buffers not initialized" error
        if (m_renderer) {
            std::cout << "Reinitializing renderer buffers after game load..." << std::endl;
            m_renderer->setupBuffers();
        }
        
        // Initialize voxel manipulator with the loaded world
        if (m_voxelManipulator) {
            m_voxelManipulator->initialize(m_world.get());
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

void Game::initializeDebugMenu() {
    // Create debug menu
    m_debugMenu = std::make_unique<Debug::DebugMenu>();
    m_debugMenu->initialize(m_window->getHandle(), this);
    
    // Register debug commands
    m_debugMenu->registerCommand("tp", "Teleport player to x y z coordinates",
        [this](const std::vector<std::string>& args) {
            if (args.size() < 4) {
                m_debugMenu->commandOutput("Usage: tp <x> <y> <z>");
                return;
            }
            
            try {
                float x = std::stof(args[1]);
                float y = std::stof(args[2]);
                float z = std::stof(args[3]);
                
                if (m_player) {
                    m_player->setPosition(glm::vec3(x, y, z));
                    m_debugMenu->commandOutput("Teleported to (" + args[1] + ", " + args[2] + ", " + args[3] + ")");
                } else {
                    m_debugMenu->commandOutput("ERROR: Player not initialized");
                }
            } catch (const std::exception& e) {
                m_debugMenu->commandOutput("ERROR: Invalid coordinates");
            }
        });
    
    m_debugMenu->registerCommand("fly", "Toggle player flying mode",
        [this](const std::vector<std::string>& args) {
            if (m_player) {
                bool isFlying = m_player->isFlying();
                m_player->setFlying(!isFlying);
                m_debugMenu->commandOutput(std::string("Flying mode ") + (!isFlying ? "ENABLED" : "DISABLED"));
            } else {
                m_debugMenu->commandOutput("ERROR: Player not initialized");
            }
        });
    
    m_debugMenu->registerCommand("noclip", "Toggle player collision",
        [this](const std::vector<std::string>& args) {
            if (m_player) {
                bool hasCollision = m_player->hasCollision();
                m_player->setCollision(!hasCollision);
                m_debugMenu->commandOutput(std::string("Collision ") + (!hasCollision ? "ENABLED" : "DISABLED"));
            } else {
                m_debugMenu->commandOutput("ERROR: Player not initialized");
            }
        });
        
    // Add renderer debug commands
    m_debugMenu->registerCommand("toggle_backface", "Toggle backface culling",
        [this](const std::vector<std::string>& args) {
            if (m_renderer) {
                bool isDisabled = m_renderer->isBackfaceCullingDisabled();
                m_renderer->setDisableBackfaceCulling(!isDisabled);
                m_debugMenu->commandOutput(std::string("Backface culling ") + 
                    (isDisabled ? "ENABLED" : "DISABLED"));
            } else {
                m_debugMenu->commandOutput("ERROR: Renderer not initialized");
            }
        });
    
    m_debugMenu->registerCommand("toggle_greedy", "Toggle greedy meshing",
        [this](const std::vector<std::string>& args) {
            if (m_renderer && m_world) {
                bool isDisabled = m_renderer->isGreedyMeshingDisabled();
                
                // Toggle the setting in both renderer and world
                m_renderer->setDisableGreedyMeshing(!isDisabled);
                m_world->setDisableGreedyMeshing(!isDisabled);
                
                m_debugMenu->commandOutput(std::string("Greedy meshing ") + 
                    (isDisabled ? "ENABLED" : "DISABLED"));
                
                // Regenerate meshes for all visible chunks
                glm::ivec3 playerChunkPos = m_world->worldToChunkPos(m_player->getPosition());
                m_debugMenu->commandOutput("Regenerating meshes for visible chunks...");
                
                // Increase the radius to ensure proper rendering of visible chunks
                const int REGEN_RADIUS = 4; // Increased from 2 to 4
                for (int x = -REGEN_RADIUS; x <= REGEN_RADIUS; x++) {
                    for (int y = -REGEN_RADIUS; y <= REGEN_RADIUS; y++) {
                        for (int z = -REGEN_RADIUS; z <= REGEN_RADIUS; z++) {
                            glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(x, y, z);
                            m_world->updateChunkMeshes(chunkPos, !isDisabled);
                        }
                    }
                }
                
                m_debugMenu->commandOutput("Mesh regeneration complete");
            } else {
                m_debugMenu->commandOutput("ERROR: Renderer or World not initialized");
            }
        });
    
    // Pass the debug menu to the renderer
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