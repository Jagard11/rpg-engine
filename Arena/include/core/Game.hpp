#pragma once

#include <memory>
#include <string>
#include "../world/World.hpp"
#include "../player/Player.hpp"
#include "../renderer/Renderer.hpp"
#include "../ui/SplashScreen.hpp"
#include "../debug/DebugMenu.hpp"
#include "../debug/DebugStats.hpp"
#include "../world/VoxelManipulator.hpp"
#include "Window.hpp"
#include <glm/glm.hpp>

class Game {
public:
    Game();
    ~Game();

    bool initialize();
    void run();
    void cleanup();

    // World management
    void createNewWorld(uint64_t seed);
    bool loadWorld(const std::string& savePath);
    void saveWorld(const std::string& savePath);

    void renderHUD();
    
    // Debug menu access
    Debug::DebugMenu* getDebugMenu() { return m_debugMenu.get(); }
    
    // Access to game components for debug stats
    World* getWorld() { return m_world.get(); }
    Player* getPlayer() { return m_player.get(); }
    int getFps() const { return m_fps; }
    
    // Handle mouse input for voxel manipulation - needs to be public for the callback
    void handleMouseInput(int button, int action, int mods);

private:
    void update(float deltaTime);
    void render();
    void handleInput(float deltaTime);
    void initializeDebugMenu();

    std::unique_ptr<Window> m_window;
    std::unique_ptr<World> m_world;
    std::unique_ptr<Player> m_player;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<SplashScreen> m_splashScreen;
    std::unique_ptr<Debug::DebugMenu> m_debugMenu;
    std::unique_ptr<Debug::DebugStats> m_debugStats;
    std::unique_ptr<VoxelManipulator> m_voxelManipulator;
    
    bool m_isRunning;
    bool m_isInGame;
    int m_fps;
    
    // Track player's chunk position to optimize updates
    glm::ivec3 m_lastPlayerChunkPos;
}; 