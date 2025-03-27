#include "ui/SplashScreen.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <random>
#include <chrono>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "core/Game.hpp"
#include "render/TextRenderer.hpp"

SplashScreen::SplashScreen()
    : m_window(nullptr)
    , m_game(nullptr)
    , m_isActive(true)
    , m_currentState(UIState::SPLASH_SCREEN)
    , m_seedInput("")
    , m_saveName("")
    , m_selectedSavePath("")
    , m_saveGames()
    , m_selectedSaveIndex(-1)
    , m_isTyping(false)
    , m_mouseX(0)
    , m_mouseY(0)
    , m_mousePressed(false)
    , m_lastMousePressed(false)
    , m_escapePressed(false)
    , m_lastEscapePressed(false)
    , m_newGameCallback(nullptr)
    , m_loadGameCallback(nullptr)
    , m_saveGameCallback(nullptr)
    , m_quitCallback(nullptr)
    , m_textRenderer(nullptr)
{
    // TextRenderer will be initialized in the initialize method when we have a valid OpenGL context
}

SplashScreen::~SplashScreen() {
}

void SplashScreen::initialize(GLFWwindow* window, Game* game) {
    m_window = window;
    m_game = game;
    std::cout << "SplashScreen initialized with window: " << window << std::endl;
    
    // Make sure we have the proper OpenGL state for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize the text renderer now that we have a valid OpenGL context
    std::cout << "Creating TextRenderer in SplashScreen::initialize" << std::endl;
    
    if (!m_textRenderer) {
        std::cout << "Creating new TextRenderer instance..." << std::endl;
        m_textRenderer = std::make_unique<Render::TextRenderer>();
    }
    
    if (!m_textRenderer->isInitialized()) {
        std::cerr << "ERROR: TextRenderer failed to initialize in SplashScreen::initialize" << std::endl;
        std::cerr << "Falling back to legacy text rendering" << std::endl;
        
        // Check OpenGL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL error during TextRenderer initialization: " << err << std::endl;
        }
    } else {
        std::cout << "TextRenderer successfully initialized in SplashScreen" << std::endl;
        
        // Update projection matrix with current window dimensions
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        std::cout << "Updating TextRenderer projection with dimensions: " << width << "x" << height << std::endl;
        m_textRenderer->updateProjection(width, height);
        
        // Test text rendering immediately with a smaller scale
        std::cout << "Testing initial text rendering..." << std::endl;
        try {
            m_textRenderer->renderText("INITIALIZATION TEST", width/2.0f, height/2.0f, 0.5f, glm::vec3(1.0f, 1.0f, 1.0f));
            
            // Check for OpenGL errors
            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                std::cerr << "OpenGL error during initial text rendering: " << err << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception during initial text rendering: " << e.what() << std::endl;
        }
    }
    
    // Initialize save games list
    refreshSaveList();
    
    // Set initial state
    m_currentState = UIState::SPLASH_SCREEN;
    m_isActive = true;
    m_selectedSaveIndex = 0;
}

void SplashScreen::render() {
    if (!m_window) {
        std::cerr << "Error: Window is null in SplashScreen::render()" << std::endl;
        return;
    }
    
    // Debug output
    static bool first_render = true;
    if (first_render) {
        std::cout << "SplashScreen rendering for the first time" << std::endl;
        first_render = false;
    }
    
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    // Update mouse position
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);
    
    // Convert mouse coordinates to match OpenGL's coordinate system
    // GLFW gives coordinates with (0,0) at top-left, we need (0,0) at bottom-left
    m_mouseX = mouseX;
    m_mouseY = height - mouseY; // Invert Y coordinate
    
    m_lastMousePressed = m_mousePressed;
    m_mousePressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    m_lastEscapePressed = m_escapePressed;
    m_escapePressed = glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    
    // Save current OpenGL state
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    
    // Set up proper OpenGL state for UI rendering
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Clear with a more vibrant background color
    glClearColor(0.2f, 0.3f, 0.8f, 1.0f); // Blue background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Update TextRenderer projection if available
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        m_textRenderer->updateProjection(width, height);
    } else {
        static bool loggedWarning = false;
        if (!loggedWarning) {
            std::cerr << "Warning: TextRenderer not initialized or not available!" << std::endl;
            loggedWarning = true;
        }
    }
    
    // Render based on current state
    switch (m_currentState) {
        case UIState::SPLASH_SCREEN:
            renderSplashScreen();
            break;
        case UIState::NEW_GAME_SCREEN:
            renderNewGameScreen();
            break;
        case UIState::LOAD_GAME_SCREEN:
            renderLoadGameScreen();
            break;
        case UIState::IN_GAME_MENU:
            renderInGameMenu();
            break;
    }
    
    // Restore OpenGL state
    glUseProgram(currentProgram);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    if (!blendEnabled) glDisable(GL_BLEND);
    glBindVertexArray(currentVAO);
}

void SplashScreen::renderSplashScreen() {
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    // Draw a prominent game title at the top
    float titleY = height * 0.85f; // Position title higher up
    float titleScale = 3.0f; // Larger scale for the title
    
    // Use TextRenderer for the title
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        // Calculate text width for centering
        float titleWidth = m_textRenderer->getTextWidth("VOXEL GAME", titleScale);
        float titleX = (width - titleWidth) / 2.0f;
        
        // Render the title with a bright color
        m_textRenderer->renderText("VOXEL GAME", titleX, titleY, titleScale, glm::vec3(0.0f, 0.8f, 1.0f));
    }
    
    // BUTTONS - Make them extra large and easy to see
    float buttonWidth = width * 0.6f; // Wider buttons
    float buttonHeight = height * 0.15f; // Taller buttons
    float buttonX = (width - buttonWidth) / 2;
    float newGameY = height * 0.6f; // Adjusted Y position
    float loadGameY = height * 0.4f; // Adjusted Y position
    float quitY = height * 0.2f; // Adjusted Y position
    
    // Check if mouse is over buttons
    bool highlightNewGame = isMouseOverButton(buttonX, newGameY, buttonWidth, buttonHeight);
    bool highlightLoadGame = isMouseOverButton(buttonX, loadGameY, buttonWidth, buttonHeight);
    bool highlightQuit = isMouseOverButton(buttonX, quitY, buttonWidth, buttonHeight);
    
    // NEW GAME BUTTON
    renderButton("NEW GAME", buttonX, newGameY, buttonWidth, buttonHeight, highlightNewGame);
    
    // LOAD GAME BUTTON
    renderButton("LOAD GAME", buttonX, loadGameY, buttonWidth, buttonHeight, highlightLoadGame);
    
    // QUIT BUTTON
    renderButton("QUIT", buttonX, quitY, buttonWidth, buttonHeight, highlightQuit);
    
    // Handle button clicks
    if (m_mousePressed && !m_lastMousePressed) {
        if (highlightNewGame) {
            m_currentState = UIState::NEW_GAME_SCREEN;
        } else if (highlightLoadGame) {
            m_currentState = UIState::LOAD_GAME_SCREEN;
        } else if (highlightQuit) {
            if (m_quitCallback) {
                m_quitCallback(true);
            }
        }
    }
}

void SplashScreen::renderNewGameScreen() {
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    // No need to set up projection - done in render method
    
    // Title - reduce scale from 6.0f to 2.5f
    glColor3f(0.8f, 0.8f, 0.8f);
    renderTextAtCenter("NEW GAME", width/2, height * 0.8f, 2.5f);
    
    // Seed input field
    float inputWidth = width * 0.6f;
    float inputHeight = height * 0.08f;
    float inputX = (width - inputWidth) / 2;
    float inputY = height * 0.6f; // Adjusted Y position
    
    // Render input field
    glColor3f(0.2f, 0.2f, 0.2f);
    renderBox(inputX, inputY, inputWidth, inputHeight);
    
    // Reduce input text scale from 3.0f to 1.5f
    glColor3f(1.0f, 1.0f, 1.0f);
    renderText(m_isTyping ? m_seedInput + "_" : m_seedInput, 
               inputX + 10, inputY + inputHeight/2, 1.5f);
    
    // Reduce help text scale from 3.0f to 1.2f
    glColor3f(0.7f, 0.7f, 0.7f);
    renderTextAtCenter("SEED (leave empty for random)", width/2, inputY + inputHeight + 20, 1.2f);
    
    // Create button
    float buttonWidth = width * 0.3f;
    float buttonHeight = height * 0.08f;
    float buttonX = (width - buttonWidth) / 2;
    float createY = height * 0.45f; // Adjusted Y position
    float backY = height * 0.3f; // Adjusted Y position
    
    bool highlightCreate = isMouseOverButton(buttonX, createY, buttonWidth, buttonHeight);
    bool highlightBack = isMouseOverButton(buttonX, backY, buttonWidth, buttonHeight);
    
    // Create world button
    renderButton("CREATE WORLD", buttonX, createY, buttonWidth, buttonHeight, highlightCreate);
    
    // Back button
    renderButton("BACK", buttonX, backY, buttonWidth, buttonHeight, highlightBack);
    
    // Handle clicking
    if (m_mousePressed && !m_lastMousePressed) {
        if (isMouseOverButton(inputX, inputY, inputWidth, inputHeight)) {
            m_isTyping = true;
        } else if (highlightCreate) {
            m_isTyping = false;
            createNewGame();
        } else if (highlightBack) {
            m_isTyping = false;
            m_currentState = UIState::SPLASH_SCREEN;
        } else {
            m_isTyping = false;
        }
    }
}

void SplashScreen::renderLoadGameScreen() {
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    // No need to set up projection - done in render method
    
    // Title - reduce scale from 6.0f to 2.5f
    glColor3f(0.8f, 0.8f, 0.8f);
    renderTextAtCenter("LOAD GAME", width/2, height * 0.9f, 2.5f);
    
    // List of save games
    float listWidth = width * 0.7f;
    float entryHeight = height * 0.08f;
    float listX = (width - listWidth) / 2;
    float listY = height * 0.7f; // Adjusted Y position, this is now the BOTTOM of the list
    float maxVisibleEntries = 5;
    
    // Draw list background
    glColor3f(0.3f, 0.3f, 0.3f);
    renderBox(listX, listY, listWidth, entryHeight * std::min(maxVisibleEntries, (float)m_saveGames.size() + 1));
    
    if (m_saveGames.empty()) {
        glColor3f(0.8f, 0.8f, 0.8f);
        renderTextAtCenter("No saved games found", width/2, listY + entryHeight/2, 1.5f);
    } else {
        // Render save entries
        for (size_t i = 0; i < m_saveGames.size(); i++) {
            if (i >= maxVisibleEntries) break;
            
            float entryY = listY + (maxVisibleEntries - 1 - i) * entryHeight; // Entries are drawn from bottom to top
            bool isSelected = m_selectedSaveIndex == (int)i;
            bool isHighlighted = isMouseOverButton(listX, entryY, listWidth, entryHeight);
            
            // Background
            if (isSelected) {
                glColor3f(0.2f, 0.3f, 0.5f);
            } else if (isHighlighted) {
                glColor3f(0.25f, 0.25f, 0.3f);
            } else {
                glColor3f(0.2f, 0.2f, 0.2f);
            }
            renderBox(listX, entryY, listWidth, entryHeight);
            
            // Save name and date - reduce scales
            glColor3f(0.8f, 0.8f, 0.8f);
            renderText(m_saveGames[i].name, listX + 10, entryY + entryHeight/2, 1.4f);
            
            glColor3f(0.6f, 0.6f, 0.6f);
            renderText(m_saveGames[i].date, listX + listWidth - 200, entryY + entryHeight/2, 1.0f);
            
            // Delete button
            float deleteWidth = 80;
            float deleteX = listX + listWidth - deleteWidth - 10;
            bool deleteHighlight = isMouseOverButton(deleteX, entryY + 5, deleteWidth, entryHeight - 10);
            
            if (deleteHighlight) {
                glColor3f(0.7f, 0.2f, 0.2f);
            } else {
                glColor3f(0.5f, 0.1f, 0.1f);
            }
            renderBox(deleteX, entryY + 5, deleteWidth, entryHeight - 10);
            
            glColor3f(0.9f, 0.9f, 0.9f);
            renderTextAtCenter("DELETE", deleteX + deleteWidth/2, entryY + entryHeight/2, 1.0f);
            
            // Handle clicking on delete
            if (m_mousePressed && !m_lastMousePressed && deleteHighlight) {
                deleteSaveGame(i);
                continue;
            }
            
            // Handle clicking on save entry
            if (m_mousePressed && !m_lastMousePressed && isHighlighted && !deleteHighlight) {
                m_selectedSaveIndex = i;
                m_selectedSavePath = m_saveGames[i].filename;
            }
        }
    }
    
    // Load and Back buttons
    float buttonWidth = width * 0.3f;
    float buttonHeight = height * 0.08f;
    float buttonX = (width - buttonWidth) / 2;
    float loadY = height * 0.4f; // Adjusted Y position
    float backY = height * 0.3f; // Adjusted Y position
    
    bool highlightLoad = isMouseOverButton(buttonX, loadY, buttonWidth, buttonHeight) && m_selectedSaveIndex >= 0;
    bool highlightBack = isMouseOverButton(buttonX, backY, buttonWidth, buttonHeight);
    
    // Load selected game button - grayed out if nothing is selected
    if (m_selectedSaveIndex >= 0) {
        renderButton("LOAD SELECTED", buttonX, loadY, buttonWidth, buttonHeight, highlightLoad);
    } else {
        // Disabled button
        glColor3f(0.4f, 0.4f, 0.4f);
        renderBox(buttonX, loadY, buttonWidth, buttonHeight);
        glColor3f(0.6f, 0.6f, 0.6f);
        renderTextAtCenter("LOAD SELECTED", buttonX + buttonWidth/2, loadY + buttonHeight/2, 1.5f);
    }
    
    // Back button
    renderButton("BACK", buttonX, backY, buttonWidth, buttonHeight, highlightBack);
    
    // Handle button clicks
    if (m_mousePressed && !m_lastMousePressed) {
        if (highlightLoad && m_selectedSaveIndex >= 0) {
            loadSelectedGame();
        } else if (highlightBack) {
            m_currentState = UIState::SPLASH_SCREEN;
            m_selectedSaveIndex = -1;
        }
    }
}

void SplashScreen::renderInGameMenu() {
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    // No need to set up projection - done in render method
    
    // Darkened background
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    renderBox(0, 0, width, height);
    glDisable(GL_BLEND);
    
    // Title - reduce scale from 6.0f to 2.5f
    glColor3f(0.9f, 0.9f, 0.9f);
    renderTextAtCenter("GAME MENU", width/2, height * 0.8f, 2.5f);
    
    // Menu buttons
    float buttonWidth = width * 0.4f;
    float buttonHeight = height * 0.08f;
    float buttonX = (width - buttonWidth) / 2;
    float buttonSpacing = height * 0.02f;
    
    float resumeY = height * 0.65f; // Adjusted Y position
    float saveY = resumeY - buttonHeight - buttonSpacing;
    float loadY = saveY - buttonHeight - buttonSpacing;
    float splashY = loadY - buttonHeight - buttonSpacing;
    float quitY = splashY - buttonHeight - buttonSpacing;
    
    bool highlightResume = isMouseOverButton(buttonX, resumeY, buttonWidth, buttonHeight);
    bool highlightSave = isMouseOverButton(buttonX, saveY, buttonWidth, buttonHeight);
    bool highlightLoad = isMouseOverButton(buttonX, loadY, buttonWidth, buttonHeight);
    bool highlightSplash = isMouseOverButton(buttonX, splashY, buttonWidth, buttonHeight);
    bool highlightQuit = isMouseOverButton(buttonX, quitY, buttonWidth, buttonHeight);
    
    // Resume button
    renderButton("RESUME GAME", buttonX, resumeY, buttonWidth, buttonHeight, highlightResume);
    
    // Save game button
    renderButton("SAVE GAME", buttonX, saveY, buttonWidth, buttonHeight, highlightSave);
    
    // Save game name input
    if (m_isTyping) {
        float inputWidth = width * 0.5f;
        float inputHeight = height * 0.06f;
        float inputX = (width - inputWidth) / 2;
        float inputY = saveY + inputHeight + 10; // Adjusted Y position
        
        // Background
        glColor3f(0.2f, 0.2f, 0.2f);
        renderBox(inputX, inputY, inputWidth, inputHeight);
        
        // Text - reduce scale from 3.0f to 1.5f
        glColor3f(1.0f, 1.0f, 1.0f);
        renderText(m_saveName + "_", inputX + 10, inputY + inputHeight/2, 1.5f);
        
        // Label - reduce scale from 3.0f to 1.2f
        glColor3f(0.9f, 0.9f, 0.9f);
        renderTextAtCenter("ENTER SAVE NAME", width/2, inputY + inputHeight + 20, 1.2f);
    }
    
    // Other buttons
    renderButton("LOAD GAME", buttonX, loadY, buttonWidth, buttonHeight, highlightLoad);
    renderButton("QUIT TO MENU", buttonX, splashY, buttonWidth, buttonHeight, highlightSplash);
    renderButton("QUIT TO DESKTOP", buttonX, quitY, buttonWidth, buttonHeight, highlightQuit);
    
    // Handle button clicks
    if (m_mousePressed && !m_lastMousePressed) {
        if (highlightResume) {
            // Resume game
            m_isActive = false;
            m_isTyping = false;
        } else if (highlightSave) {
            if (!m_isTyping) {
                m_isTyping = true;
                m_saveName = "World_" + getCurrentDateString();
            } else {
                // Confirm save with current name
                saveCurrentGame();
                m_isTyping = false;
            }
        } else if (highlightLoad) {
            m_currentState = UIState::LOAD_GAME_SCREEN;
            refreshSaveList();
            m_isTyping = false;
        } else if (highlightSplash) {
            // Quit to main menu
            m_currentState = UIState::SPLASH_SCREEN;
            if (m_quitCallback) {
                m_quitCallback(false); // false = quit to menu, not desktop
            }
            m_isTyping = false;
        } else if (highlightQuit) {
            // Quit to desktop
            if (m_quitCallback) {
                m_quitCallback(true); // true = quit to desktop
            }
            m_isTyping = false;
        } else {
            m_isTyping = false;
        }
    }
}

void SplashScreen::renderTextAtCenter(const std::string& text, float centerX, float centerY, float scale) {
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        // Calculate text width for centering
        float width = m_textRenderer->getTextWidth(text, scale);
        
        // Calculate x position for centering
        float x = centerX - (width / 2.0f);
        
        // Render the text
        m_textRenderer->renderText(text, x, centerY, scale, glm::vec3(1.0f, 1.0f, 1.0f));
    }
    // No fallback rendering - if TextRenderer isn't available, nothing will be rendered
}

void SplashScreen::renderText(const std::string& text, float x, float y, float scale) {
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        m_textRenderer->renderText(text, x, y, scale, glm::vec3(1.0f, 1.0f, 1.0f));
    }
    // No fallback rendering - if TextRenderer isn't available, nothing will be rendered
}

void SplashScreen::renderButton(const std::string& text, float x, float y, float width, float height, bool highlight) {
    // Draw button background
    glColor4f(0.2f, 0.2f, 0.2f, 0.8f); // Semi-transparent dark background
    if (highlight) {
        glColor4f(0.3f, 0.3f, 0.3f, 0.9f); // Slightly lighter when highlighted
    }
    renderBox(x, y, width, height);
    
    // Draw button border
    glColor4f(0.8f, 0.8f, 0.8f, 1.0f); // Light border
    if (highlight) {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Brighter border when highlighted
    }
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
    
    // Render button text - reduce scale from 1.5f to 1.0f
    if (m_textRenderer && m_textRenderer->isInitialized()) {
        float textScale = 1.0f;
        float textWidth = m_textRenderer->getTextWidth(text, textScale);
        float textX = x + (width - textWidth) / 2.0f;
        float textY = y + (height + textScale * 20.0f) / 2.0f; // Center text vertically
        
        glm::vec3 textColor = highlight ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(0.8f, 0.8f, 0.8f);
        m_textRenderer->renderText(text, textX, textY, textScale, textColor);
    }
}

void SplashScreen::renderBox(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

void SplashScreen::handleInput(int key, int action) {
    if (!m_isActive) return;
    
    // Handle escape key
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        m_escapePressed = true;
    } else if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        m_escapePressed = false;
    }
    
    if (m_isTyping) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            // Handle backspace
            if (key == GLFW_KEY_BACKSPACE) {
                if (m_currentState == UIState::NEW_GAME_SCREEN && !m_seedInput.empty()) {
                    m_seedInput.pop_back();
                } else if ((m_currentState == UIState::LOAD_GAME_SCREEN || 
                            m_currentState == UIState::IN_GAME_MENU) && 
                            !m_saveName.empty()) {
                    m_saveName.pop_back();
                }
            }
            // Handle enter to complete typing
            else if (key == GLFW_KEY_ENTER) {
                m_isTyping = false;
                if (m_currentState == UIState::NEW_GAME_SCREEN) {
                    createNewGame();
                } else if (m_currentState == UIState::IN_GAME_MENU) {
                    saveCurrentGame();
                }
            }
            // Allow only alphanumeric characters, handled by characterCallback
            // This is just for input mode detection
            else if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z) ||
                    (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) ||
                    key == GLFW_KEY_SPACE) {
                // Character input handled by callback
            }
        }
    } else {
        // Not typing, handle regular menu navigation
        if (action == GLFW_PRESS) {
            handleMenuNavigation(key);
        }
    }
}

void SplashScreen::characterCallback(unsigned int codepoint) {
    if (!m_isActive || !m_isTyping) return;
    
    // Append character to input field if it's a printable character
    if (codepoint >= 32 && codepoint < 127) {
        std::string& inputField = (m_currentState == UIState::NEW_GAME_SCREEN) ? m_seedInput : m_saveName;
        inputField += (char)codepoint;
    }
}

void SplashScreen::activateInGameMenu() {
    m_currentState = UIState::IN_GAME_MENU;
    m_isActive = true;
}

void SplashScreen::setInactive() {
    m_isActive = false;
}

bool SplashScreen::isActive() const {
    return m_isActive;
}

void SplashScreen::setNewGameCallback(std::function<void(long)> callback) {
    m_newGameCallback = callback;
}

void SplashScreen::setLoadGameCallback(std::function<void(const std::string&)> callback) {
    m_loadGameCallback = callback;
}

void SplashScreen::setSaveGameCallback(std::function<void(const std::string&)> callback) {
    m_saveGameCallback = callback;
}

void SplashScreen::setQuitCallback(std::function<void(bool)> callback) {
    m_quitCallback = callback;
}

void SplashScreen::createNewGame() {
    if (m_newGameCallback) {
        long seed;
        if (m_seedInput.empty()) {
            // Generate random seed
            seed = std::chrono::system_clock::now().time_since_epoch().count();
        } else {
            try {
                seed = std::stol(m_seedInput);
            } catch (const std::exception&) {
                // If not a valid number, hash the string to create a seed
                std::hash<std::string> hasher;
                seed = hasher(m_seedInput);
            }
        }
        
        m_newGameCallback(seed);
        m_isActive = false;
    }
    m_seedInput.clear();
}

void SplashScreen::loadSelectedGame() {
    if (m_loadGameCallback && m_selectedSaveIndex >= 0 && 
        m_selectedSaveIndex < (int)m_saveGames.size()) {
        
        m_loadGameCallback(m_saveGames[m_selectedSaveIndex].filename);
        m_isActive = false;
        m_currentState = UIState::SPLASH_SCREEN;
    }
}

void SplashScreen::saveCurrentGame() {
    if (m_saveGameCallback && !m_saveName.empty()) {
        // Create filename from save name
        std::string cleanName = m_saveName;
        
        // Replace spaces and invalid chars with underscores
        for (char& c : cleanName) {
            if (!std::isalnum(c)) {
                c = '_';
            }
        }
        
        std::string filename = "saves/" + cleanName + ".sav";
        m_saveGameCallback(filename);
        
        // Add this save to the list
        refreshSaveList();
        m_isActive = false;
    }
}

void SplashScreen::deleteSaveGame(int index) {
    if (index >= 0 && index < (int)m_saveGames.size()) {
        std::filesystem::path savePath = m_saveGames[index].filename;
        
        try {
            if (std::filesystem::exists(savePath)) {
                std::filesystem::remove(savePath);
                std::cout << "Deleted save file: " << savePath << std::endl;
                
                // Update the selected index if necessary
                if (index == m_selectedSaveIndex) {
                    m_selectedSaveIndex = -1;
                    m_selectedSavePath = "";
                }
                
                // Refresh the list
                refreshSaveList();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error deleting save file: " << e.what() << std::endl;
        }
    }
}

std::string SplashScreen::getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    
    std::tm* timeinfo = std::localtime(&time);
    char buffer[80];
    std::strftime(buffer, 80, "%Y%m%d_%H%M%S", timeinfo);
    
    return std::string(buffer);
}

void SplashScreen::handleMenuNavigation(int key) {
    // Handle menu navigation based on current state
    switch (m_currentState) {
        case UIState::SPLASH_SCREEN:
            // Main menu navigation
            if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
                // Simple option cycling through the main menu options (New Game, Load Game, Quit)
                // We have 3 options in the splash screen
                int optionCount = 3;
                int currentOption = m_selectedSaveIndex;
                
                if (key == GLFW_KEY_UP) {
                    currentOption = (currentOption - 1 + optionCount) % optionCount;
                } else {
                    currentOption = (currentOption + 1) % optionCount;
                }
                
                m_selectedSaveIndex = currentOption;
            } else if (key == GLFW_KEY_ENTER) {
                // Select the current option
                switch (m_selectedSaveIndex) {
                    case 0: // New Game
                        m_currentState = UIState::NEW_GAME_SCREEN;
                        m_seedInput = "";
                        m_isTyping = true;
                        break;
                    case 1: // Load Game
                        m_currentState = UIState::LOAD_GAME_SCREEN;
                        refreshSaveList();
                        break;
                    case 2: // Quit
                        if (m_quitCallback) {
                            m_quitCallback(true);
                        }
                        break;
                }
            }
            break;
            
        case UIState::LOAD_GAME_SCREEN:
            // Load game screen navigation
            if (key == GLFW_KEY_UP) {
                if (m_selectedSaveIndex > 0) {
                    m_selectedSaveIndex--;
                }
            } else if (key == GLFW_KEY_DOWN) {
                if (m_selectedSaveIndex < static_cast<int>(m_saveGames.size()) - 1) {
                    m_selectedSaveIndex++;
                }
            } else if (key == GLFW_KEY_ENTER) {
                if (m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(m_saveGames.size())) {
                    loadSelectedGame();
                }
            } else if (key == GLFW_KEY_DELETE) {
                if (m_selectedSaveIndex >= 0 && m_selectedSaveIndex < static_cast<int>(m_saveGames.size())) {
                    deleteSaveGame(m_selectedSaveIndex);
                }
            } else if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_ESCAPE) {
                // Go back to main menu
                m_currentState = UIState::SPLASH_SCREEN;
                m_selectedSaveIndex = 0;
            }
            break;
            
        case UIState::IN_GAME_MENU:
            // In-game menu navigation
            if (key == GLFW_KEY_UP || key == GLFW_KEY_DOWN) {
                // Cycle through menu options (Resume, Save, Quit)
                int optionCount = 3;
                int currentOption = m_selectedSaveIndex;
                
                if (key == GLFW_KEY_UP) {
                    currentOption = (currentOption - 1 + optionCount) % optionCount;
                } else {
                    currentOption = (currentOption + 1) % optionCount;
                }
                
                m_selectedSaveIndex = currentOption;
            } else if (key == GLFW_KEY_ENTER) {
                switch (m_selectedSaveIndex) {
                    case 0: // Resume
                        m_isActive = false;
                        break;
                    case 1: // Save
                        m_saveName = "save";  // Default name
                        m_isTyping = true;
                        break;
                    case 2: // Quit
                        if (m_quitCallback) {
                            m_quitCallback(false);
                        }
                        break;
                }
            } else if (key == GLFW_KEY_ESCAPE) {
                // Resume game
                m_isActive = false;
            }
            break;
            
        case UIState::NEW_GAME_SCREEN:
            // New game screen
            if (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_ESCAPE) {
                // Go back to main menu
                m_currentState = UIState::SPLASH_SCREEN;
                m_selectedSaveIndex = 0;
            }
            break;
    }
}

bool SplashScreen::isMouseOverButton(float x, float y, float width, float height) const {
    return (m_mouseX >= x && m_mouseX <= x + width &&
            m_mouseY >= y && m_mouseY <= y + height);
}

void SplashScreen::refreshSaveList() {
    m_saveGames.clear();
    
    // Get all files in the save directory
    const std::string saveDir = "saves/";
    
    // Create the directory if it doesn't exist
    std::filesystem::create_directories(saveDir);
    
    for (const auto& entry : std::filesystem::directory_iterator(saveDir)) {
        if (entry.path().extension() == ".sav") {
            SaveGame saveInfo;
            saveInfo.filename = entry.path().string();
            
            // Extract name from filename (remove path and extension)
            std::string filename = entry.path().filename().string();
            saveInfo.name = filename.substr(0, filename.find_last_of('.'));
            
            // Get last modified time
            try {
                // Get last modified time using C functions for compatibility
                auto lwt = std::filesystem::last_write_time(entry.path());
                auto now = std::filesystem::file_time_type::clock::now();
                if (lwt < now) {
                    // Convert to a string representation
                    std::time_t ctime = std::time(nullptr);
                    std::tm* timeinfo = std::localtime(&ctime);
                    
                    char buffer[32];
                    std::strftime(buffer, 32, "%Y-%m-%d %H:%M:%S", timeinfo);
                    saveInfo.date = buffer;
                } else {
                    saveInfo.date = "Invalid date";
                }
            } catch (const std::exception& e) {
                saveInfo.date = "Unknown date";
            }
            
            m_saveGames.push_back(saveInfo);
        }
    }
    
    // Sort by most recent (alphabetically for now since we are not handling dates properly)
    std::sort(m_saveGames.begin(), m_saveGames.end(), [](const SaveGame& a, const SaveGame& b) {
        return a.name > b.name;
    });
    
    // Reset selection
    m_selectedSaveIndex = m_saveGames.empty() ? -1 : 0;
} 