#pragma once

#include <string>
#include <functional>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>

// Forward declarations
class Game;

// UI states for navigation
enum class UIState {
    SPLASH_SCREEN,
    NEW_GAME_SCREEN,
    LOAD_GAME_SCREEN,
    IN_GAME_MENU
};

struct SaveGame {
    std::string name;
    std::string filename;
    std::string date;
};

class SplashScreen {
public:
    SplashScreen();
    ~SplashScreen();

    void initialize(GLFWwindow* window, Game* game);
    void render();
    void handleInput(int key, int action);
    bool isActive() const;
    void setInactive();
    
    void activateInGameMenu();
    void characterCallback(unsigned int codepoint);

    // Callbacks for main game
    void setNewGameCallback(std::function<void(long)> callback);
    void setLoadGameCallback(std::function<void(const std::string&)> callback);
    void setSaveGameCallback(std::function<void(const std::string&)> callback);
    void setQuitCallback(std::function<void(bool)> callback);

    void setCurrentState(UIState state) { m_currentState = state; }
    UIState getCurrentState() const { return m_currentState; }

    // Save/load game functionality
    void refreshSaveList();
    void deleteSave(const std::string& filename);

private:
    // Main render methods for different states
    void renderSplashScreen();
    void renderNewGameScreen();
    void renderLoadGameScreen();
    void renderInGameMenu();
    
    // Helper methods
    bool isMouseOverButton(float x, float y, float width, float height) const;
    void renderButton(const std::string& text, float x, float y, float width, float height, bool highlight);
    void renderBox(float x, float y, float width, float height);
    void renderText(const std::string& text, float x, float y, float scale);
    void renderTextAtCenter(const std::string& text, float centerX, float centerY, float scale);
    void renderCharacter(char c, float x, float y, float width, float height, float scale);
    void handleMenuNavigation(int key);
    
    // Game state management
    void createNewGame();
    void loadSelectedGame();
    void saveCurrentGame();
    void deleteSaveGame(int index);
    std::string getCurrentDateString();

    GLFWwindow* m_window;
    Game* m_game;
    bool m_isActive;
    
    // UI state
    UIState m_currentState;
    std::string m_seedInput;
    std::string m_saveName;
    std::string m_selectedSavePath;
    std::vector<SaveGame> m_saveGames;
    int m_selectedSaveIndex;
    
    // Input state
    bool m_isTyping;
    double m_mouseX, m_mouseY;
    bool m_mousePressed;
    bool m_lastMousePressed;
    bool m_escapePressed;
    bool m_lastEscapePressed;
    
    // Callbacks
    std::function<void(long)> m_newGameCallback;
    std::function<void(const std::string&)> m_loadGameCallback;
    std::function<void(const std::string&)> m_saveGameCallback;
    std::function<void(bool)> m_quitCallback;
}; 