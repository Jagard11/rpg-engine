// ./include/Core/GameStateManager.hpp
#ifndef GAME_STATE_MANAGER_HPP
#define GAME_STATE_MANAGER_HPP

#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <iostream>

// Forward declarations
class Player;
class World;
class InputHandler;
class Renderer;

// Enum for game states
enum class GameState {
    LOADING,      // Initial loading screen
    PLAYING,      // Normal gameplay
    PAUSED,       // Game paused with menu showing
    SETTINGS,     // Settings menu open
    DEBUG,        // Debug interface showing
    INVENTORY     // Full inventory open (not just hotbar)
};

/**
 * Manages game state transitions and state-specific behaviors
 * Centralizes the control of what happens in each game state
 */
class GameStateManager {
public:
    // Singleton instance accessor
    static GameStateManager& getInstance() {
        static GameStateManager instance;
        return instance;
    }
    
    // Initialize the state manager
    void initialize() {
        currentState = GameState::LOADING;
        loadingProgress = 0.0f;
        
        // Register state change callbacks
        registerStateChangeCallback("default", [this](GameState from, GameState to) {
            std::cout << "State changed from " << getStateName(from) << " to " << getStateName(to) << std::endl;
        });
    }
    
    // Update loading progress
    void updateLoadingProgress(float progress) {
        loadingProgress = progress;
        if (loadingProgress >= 1.0f && currentState == GameState::LOADING) {
            changeState(GameState::PLAYING);
        }
    }
    
    // Get current loading progress
    float getLoadingProgress() const {
        return loadingProgress;
    }
    
    // Get current game state
    GameState getCurrentState() const {
        return currentState;
    }
    
    // Change to a new state
    void changeState(GameState newState) {
        if (newState == currentState) return;
        
        GameState oldState = currentState;
        currentState = newState;
        
        // Call all registered state change callbacks
        for (const auto& callback : stateChangeCallbacks) {
            callback.second(oldState, newState);
        }
    }
    
    // Register a callback for state changes
    void registerStateChangeCallback(const std::string& id, std::function<void(GameState, GameState)> callback) {
        stateChangeCallbacks[id] = callback;
    }
    
    // Remove a state change callback
    void unregisterStateChangeCallback(const std::string& id) {
        stateChangeCallbacks.erase(id);
    }
    
    // Check if a specific UI element should be shown
    bool shouldShowUI(const std::string& uiElement) const {
        if (uiElement == "loading_screen") {
            return currentState == GameState::LOADING;
        }
        else if (uiElement == "pause_menu") {
            return currentState == GameState::PAUSED;
        }
        else if (uiElement == "settings_menu") {
            return currentState == GameState::SETTINGS;
        }
        else if (uiElement == "debug_window") {
            return currentState == GameState::DEBUG;
        }
        else if (uiElement == "inventory") {
            return currentState == GameState::INVENTORY;
        }
        else if (uiElement == "hotbar") {
            return currentState == GameState::PLAYING || 
                   currentState == GameState::DEBUG ||
                   currentState == GameState::INVENTORY;
        }
        else if (uiElement == "crosshair") {
            return currentState == GameState::PLAYING || 
                   currentState == GameState::DEBUG;
        }
        
        return false;
    }
    
    // Check if player input should be processed
    bool shouldProcessPlayerInput() const {
        return currentState == GameState::PLAYING || 
               currentState == GameState::DEBUG;
    }
    
    // Check if world updates should occur
    bool shouldUpdateWorld() const {
        return currentState == GameState::PLAYING || 
               currentState == GameState::DEBUG;
    }
    
    // Helper to get state name
    std::string getStateName(GameState state) const {
        switch (state) {
            case GameState::LOADING: return "LOADING";
            case GameState::PLAYING: return "PLAYING";
            case GameState::PAUSED: return "PAUSED";
            case GameState::SETTINGS: return "SETTINGS";
            case GameState::DEBUG: return "DEBUG";
            case GameState::INVENTORY: return "INVENTORY";
            default: return "UNKNOWN";
        }
    }
    
    // Toggle debug mode
    void toggleDebugMode() {
        if (currentState == GameState::DEBUG) {
            changeState(GameState::PLAYING);
        } else {
            changeState(GameState::DEBUG);
        }
    }
    
    // Toggle pause menu
    void togglePauseMenu() {
        if (currentState == GameState::PAUSED) {
            changeState(GameState::PLAYING);
        } else if (currentState == GameState::PLAYING) {
            changeState(GameState::PAUSED);
        }
    }
    
    // Toggle settings menu
    void toggleSettingsMenu() {
        if (currentState == GameState::SETTINGS) {
            changeState(GameState::PLAYING);
        } else {
            changeState(GameState::SETTINGS);
        }
    }
    
    // Toggle inventory
    void toggleInventory() {
        if (currentState == GameState::INVENTORY) {
            changeState(GameState::PLAYING);
        } else if (currentState == GameState::PLAYING) {
            changeState(GameState::INVENTORY);
        }
    }

private:
    // Private constructor for singleton
    GameStateManager() : currentState(GameState::LOADING), loadingProgress(0.0f) {}
    
    // Prevent copying
    GameStateManager(const GameStateManager&) = delete;
    GameStateManager& operator=(const GameStateManager&) = delete;
    
    // Current game state
    GameState currentState;
    
    // Loading progress (0.0 to 1.0)
    float loadingProgress;
    
    // Callbacks for state changes
    std::unordered_map<std::string, std::function<void(GameState, GameState)>> stateChangeCallbacks;
};

#endif // GAME_STATE_MANAGER_HPP