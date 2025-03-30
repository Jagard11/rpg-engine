#include "debug/DebugStats.hpp"
#include "core/Game.hpp"
#include "world/World.hpp"
#include "player/Player.hpp"
#include "render/TextRenderer.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace Debug {

DebugStats::DebugStats()
    : m_window(nullptr)
    , m_game(nullptr)
    , m_textRenderer(nullptr)
    , m_isVisible(true)
    , m_currentFps(0)
    , m_highFps(0)
    , m_lowFps(999999.0f)
    , m_avgFps(0)
    , m_fpsAccumulator(0)
    , m_fpsFrameCount(0)
    , m_lastUpdateTime(0.0f)
    , m_loadedChunks(0)
    , m_visibleChunks(0)
    , m_pendingChunks(0)
    , m_dirtyChunks(0)
{
}

DebugStats::~DebugStats() {
    // Clean up resources if needed
}

void DebugStats::initialize(GLFWwindow* window, Game* game) {
    m_window = window;
    m_game = game;
    
    // Initialize text renderer
    m_textRenderer = std::make_unique<Render::TextRenderer>();
    // The TextRenderer is already initialized in its constructor
    
    // Pre-fill FPS history with zeros
    m_fpsHistory.assign(60, 0.0f); // 60 seconds of history
}

void DebugStats::update(float deltaTime) {
    if (!m_isVisible) return;
    
    // Update FPS stats
    updateFpsStats(deltaTime);
    
    // Update chunk stats
    if (m_game && m_game->getWorld()) {
        World* world = m_game->getWorld();
        m_loadedChunks = world->getChunks().size();
        m_visibleChunks = world->getVisibleChunksCount();
        m_pendingChunks = world->getPendingChunksCount();
        m_dirtyChunks = world->getDirtyChunkCount();
    } else {
        m_loadedChunks = 0;
        m_visibleChunks = 0;
        m_pendingChunks = 0;
        m_dirtyChunks = 0;
    }
    
    // Update other stats as needed
    m_lastUpdateTime += deltaTime;
}

void DebugStats::updateFpsStats(float deltaTime) {
    // Increment frame counter for current second
    m_fpsFrameCount++;
    m_fpsAccumulator += deltaTime;
    
    // Update FPS counters every second
    if (m_fpsAccumulator >= 1.0f) {
        // Calculate current FPS
        m_currentFps = m_fpsFrameCount;
        
        // Update history
        m_fpsHistory.pop_front();
        m_fpsHistory.push_back(static_cast<float>(m_currentFps));
        
        // Calculate high, low, and average FPS over history
        m_highFps = *std::max_element(m_fpsHistory.begin(), m_fpsHistory.end());
        
        // Only consider non-zero values for low FPS calculation
        m_lowFps = 9999.0f;
        for (float fps : m_fpsHistory) {
            if (fps > 0.0f && fps < m_lowFps) {
                m_lowFps = fps;
            }
        }
        if (m_lowFps == 9999.0f) m_lowFps = 0.0f;
        
        // Calculate average FPS (only consider non-zero values)
        int nonZeroCount = 0;
        float sum = 0.0f;
        for (float fps : m_fpsHistory) {
            if (fps > 0.0f) {
                sum += fps;
                nonZeroCount++;
            }
        }
        m_avgFps = nonZeroCount > 0 ? sum / nonZeroCount : 0.0f;
        
        // Reset counters
        m_fpsFrameCount = 0;
        m_fpsAccumulator -= 1.0f; // Subtract one second (keeps remainder)
    }
}

void DebugStats::render() {
    std::cout << "[DebugStats] Render called. m_isVisible = " << m_isVisible << std::endl;
    
    // No longer force visibility for debugging
    // m_isVisible = true;
    
    if (!m_isVisible || !m_textRenderer || !m_window || !m_game) {
        std::cout << "[DebugStats] Render exiting early. Conditions: isVisible=" << m_isVisible 
                  << ", textRenderer=" << (m_textRenderer ? "true" : "false") 
                  << ", window=" << (m_window ? "true" : "false") 
                  << ", game=" << (m_game ? "true" : "false") << std::endl;
        return;
    }
    
    std::cout << "[DebugStats] Passed visibility check, proceeding with rendering" << std::endl;
    
    // Get window dimensions
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    
    // Save current GL state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint previousBlendSrc, previousBlendDst;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDst);
    
    // Set up orthographic projection for 2D drawing
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    // IMPORTANT: Use bottom-to-top projection for debug stats to match our text drawing coordinates
    glOrtho(0, width, 0, height, -1, 1);  // Changed from (0,height) to (0,0) at bottom-left
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Prepare OpenGL for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Define panel dimensions and position
    float panelWidth = 450.0f; // Slightly wider for separate chunk lines
    float panelHeight = 190.0f; // Increased height for more lines
    float panelX = 5.0f;
    float panelY = height - 5.0f - panelHeight; // Position from top-left, adjust y based on height

    // Draw a semi-transparent black background rectangle
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(panelX, panelY + panelHeight); // Top-left
    glVertex2f(panelX + panelWidth, panelY + panelHeight); // Top-right
    glVertex2f(panelX + panelWidth, panelY); // Bottom-right
    glVertex2f(panelX, panelY); // Bottom-left
    glEnd();
    
    // Draw a border around the debug panel
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(panelX, panelY + panelHeight); // Top-left
    glVertex2f(panelX + panelWidth, panelY + panelHeight); // Top-right
    glVertex2f(panelX + panelWidth, panelY); // Bottom-right
    glVertex2f(panelX, panelY); // Bottom-left
    glEnd();
    
    // Get debug info
    int fps = m_currentFps;
    int avgFps = static_cast<int>(m_avgFps);
    int highFps = static_cast<int>(m_highFps);
    int lowFps = static_cast<int>(m_lowFps);
    
    glm::ivec3 playerPos;
    glm::ivec3 playerChunkPos;
    if (m_game->getPlayer()) {
        const glm::vec3& pos = m_game->getPlayer()->getPosition();
        playerPos = glm::ivec3(
            static_cast<int>(floor(pos.x)),
            static_cast<int>(floor(pos.y)),
            static_cast<int>(floor(pos.z))
        );
        if (m_game->getWorld()) {
             playerChunkPos = m_game->getWorld()->worldToChunkPos(pos);
        }
    }
    
    int loadedChunks = m_loadedChunks;
    int visibleChunks = m_visibleChunks;
    int pendingChunks = m_pendingChunks;
    int dirtyChunks = m_dirtyChunks;
    int viewDistance = 0;
    bool greedyMeshing = false;
    if (m_game->getWorld()) {
        viewDistance = m_game->getWorld()->getViewDistance();
        greedyMeshing = m_game->getWorld()->isGreedyMeshingEnabled();
    }
    
    // Draw text info using manual drawing
    // Text positioning - Adjust Y based on new panel position
    float baseX = panelX + 10.0f; 
    float lineHeight = 20.0f; // Slightly reduced line height
    float startY = panelY + panelHeight - 20.0f; // Start from the top of the panel
    
    // Title line
    glColor3f(0.7f, 0.7f, 0.7f);
    drawManualText("VoxelGame Stats (F9 to hide)", baseX, startY);
    startY -= lineHeight;
    
    // FPS info
    std::stringstream fpsStr;
    fpsStr << "FPS: " << fps << " (Avg: " << avgFps << ", High: " << highFps << ", Low: " << lowFps << ")";
    glColor3f(1.0f, 1.0f, 0.0f);
    drawManualText(fpsStr.str(), baseX, startY);
    startY -= lineHeight;
    
    // Player position
    std::stringstream posStr;
    posStr << "Player Pos: [" << playerPos.x << ", " << playerPos.y << ", " << playerPos.z << "]";
    glColor3f(0.0f, 1.0f, 1.0f);
    drawManualText(posStr.str(), baseX, startY);
    startY -= lineHeight;

    // Player Chunk position
    std::stringstream playerChunkStr;
    playerChunkStr << "Player Chunk: [" << playerChunkPos.x << ", " << playerChunkPos.y << ", " << playerChunkPos.z << "]";
    glColor3f(0.0f, 1.0f, 1.0f);
    drawManualText(playerChunkStr.str(), baseX, startY);
    startY -= lineHeight;
    
    // Chunk information - Separate lines
    std::stringstream loadedStr; loadedStr << "Loaded Chunks: " << loadedChunks;
    glColor3f(0.5f, 1.0f, 0.5f);
    drawManualText(loadedStr.str(), baseX, startY); startY -= lineHeight;
    
    std::stringstream visibleStr; visibleStr << "Visible Chunks: " << visibleChunks;
    drawManualText(visibleStr.str(), baseX, startY); startY -= lineHeight;
    
    std::stringstream pendingStr; pendingStr << "Pending Chunks: " << pendingChunks;
    drawManualText(pendingStr.str(), baseX, startY); startY -= lineHeight;
    
    std::stringstream dirtyStr; dirtyStr << "Dirty Chunks: " << dirtyChunks;
    drawManualText(dirtyStr.str(), baseX, startY); startY -= lineHeight;
    
    // View distance & Greedy Meshing
    std::stringstream settingsStr;
    settingsStr << "View: " << viewDistance << " chunks, Greedy: " << (greedyMeshing ? "ON" : "OFF");
    glColor3f(0.8f, 0.8f, 1.0f);
    drawManualText(settingsStr.str(), baseX, startY);
    
    // Restore matrix state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    std::cout << "[DebugStats] Completed rendering debug stats" << std::endl;
    
    // Restore OpenGL state
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
        
    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
        
    glBlendFunc(previousBlendSrc, previousBlendDst);
}

// Helper method to draw text manually using OpenGL
void DebugStats::drawManualText(const std::string& text, float x, float y) {
    float baseCharWidth = 8.0f;  // Base width for most characters
    float charHeight = 15.0f;
    float spacing = 2.0f;  // Reduced spacing for better text density
    
    glLineWidth(1.5f);
    glPointSize(2.0f);  // Improved point size for dots
    
    float xPos = x;  // Start position
    
    for (size_t i = 0; i < text.length(); i++) {
        char c = text[i];
        float charWidth = baseCharWidth;  // Reset to base width for each character
        
        // Adjust width for specific characters
        if (c == 'i' || c == 'l' || c == 'I' || c == '.' || c == ',' || c == ':' || c == ';' || c == '\'')
            charWidth = baseCharWidth * 0.6f;  // Narrower characters
        else if (c == 'm' || c == 'w' || c == 'W' || c == 'M')
            charWidth = baseCharWidth * 1.2f;  // Wider characters
        
        // IMPORTANT: For all vertex calculations, invert the y-coordinate by using (y + value) instead of (y - value)
        // and (y - value) instead of (y + value) to fix the upside-down rendering
        
        // Simple implementation for common characters
        switch (c) {
            case 'A':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);  // Bottom-left
                glVertex2f(xPos + charWidth/2, y + charHeight); // Top-center
                glVertex2f(xPos + charWidth/2, y + charHeight); // Top-center
                glVertex2f(xPos + charWidth, y); // Bottom-right
                glVertex2f(xPos + charWidth/4, y + charHeight/2); // Middle-left
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2); // Middle-right
                glEnd();
                break;
                
            case 'B':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y - charHeight);
                glVertex2f(xPos, y - charHeight);
                glVertex2f(xPos + charWidth*3/4, y - charHeight);
                glVertex2f(xPos + charWidth, y - charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y - charHeight/2);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos, y);
                glEnd();
                break;
                
            case 'C':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'D':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth*2/3, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*2/3);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth*2/3, y);
                glVertex2f(xPos, y);
                glEnd();
                break;
                
            case 'E':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/2);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'F':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/2);
                glEnd();
                break;
                
            case 'G':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'H':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case 'I':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'J':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/4);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/4, y + charHeight/4);
                glEnd();
                break;
                
            case 'K':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'L':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'M':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth/3, y + charHeight/3);
                glVertex2f(xPos + charWidth/3, y);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*2/3, y);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'N':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'O':
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos, y + charHeight/4);
                glEnd();
                break;
                
            case 'P':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glEnd();
                break;
                
            case 'Q':
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos, y + charHeight/4);
                glEnd();
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*4/3, y);
                glEnd();
                break;
                
            case 'R':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glEnd();
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'S':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos, y);
                glEnd();
                break;
                
            case 'T':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/2, y);
                glEnd();
                break;
                
            case 'U':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'V':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'W':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case 'X':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'Y':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y);
                glEnd();
                break;
                
            case 'Z':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case '0':
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos, y + charHeight/4);
                glEnd();
                break;
                
            case '1':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case '2':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case '3':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos, y);
                glEnd();
                break;
                
            case '4':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case '5':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth/4, y);
                glEnd();
                break;
                
            case '6':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glEnd();
                break;
                
            case '7':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth/2, y);
                glEnd();
                break;
                
            case '8':
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glEnd();
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glEnd();
                break;
                
            case '9':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case '[':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case ']':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos, y);
                glEnd();
                break;
                
            case ':':
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight/4);
                glEnd();
                break;
                
            case ',':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/2, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y);
                glEnd();
                break;
                
            case '.':
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y + charHeight/5);
                glEnd();
                break;
                
            case ' ':
                // Just skip space
                break;
                
            case '(':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*2/3, y + charHeight);
                glVertex2f(xPos + charWidth/3, y + charHeight*2/3);
                glVertex2f(xPos + charWidth/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*2/3, y);
                glEnd();
                break;
                
            case ')':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/3, y + charHeight);
                glVertex2f(xPos + charWidth*2/3, y + charHeight*2/3);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth/3, y);
                glEnd();
                break;
                
            case '-':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case '+':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glEnd();
                break;
            
            case '=':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight*2/3);
                glVertex2f(xPos + charWidth, y + charHeight*2/3);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glEnd();
                break;
                
            case '/':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case '\\':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            // Lowercase letters
            case 'a':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case 'b':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glEnd();
                break;
                
            case 'c':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos, y + charHeight/6);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/6);
                glEnd();
                break;
                
            case 'd':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case 'e':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos, y + charHeight/6);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/6);
                glEnd();
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glEnd();
                break;
                
            case 'f':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*3/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y);
                glEnd();
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glEnd();
                break;
                
            case 'g':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos + charWidth/4, y + charHeight/6);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/6);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth, y - charHeight/6);
                glVertex2f(xPos + charWidth*3/4, y - charHeight/3);
                glVertex2f(xPos + charWidth/4, y - charHeight/3);
                glVertex2f(xPos, y - charHeight/6);
                glEnd();
                break;
                
            case 'h':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'i':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glEnd();
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y + charHeight*2/3);
                glEnd();
                break;
                
            case 'j':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/2);
                glVertex2f(xPos + charWidth*2/3, y - charHeight/6);
                glVertex2f(xPos + charWidth/2, y - charHeight/3);
                glVertex2f(xPos + charWidth/3, y - charHeight/6);
                glEnd();
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth*2/3, y + charHeight*2/3);
                glEnd();
                break;
                
            case 'k':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/4);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'l':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glEnd();
                break;
                
            case 'm':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth/3, y + charHeight/3);
                glVertex2f(xPos + charWidth/3, y);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*2/3, y);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'n':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'o':
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/6);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos, y + charHeight/6);
                glEnd();
                break;
                
            case 'p':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y - charHeight/3);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos, y);
                glEnd();
                break;
                
            case 'q':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y - charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'r':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/3);
                glEnd();
                break;
                
            case 's':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/3);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/6);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos, y + charHeight/6);
                glEnd();
                break;
                
            case 't':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight/6);
                glVertex2f(xPos + charWidth*3/4, y);
                glEnd();
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/4, y + charHeight*2/3);
                glVertex2f(xPos + charWidth*3/4, y + charHeight*2/3);
                glEnd();
                break;
                
            case 'u':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/6);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth, y + charHeight/6);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case 'v':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case 'w':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/5, y);
                glVertex2f(xPos + charWidth*2/5, y + charHeight/3);
                glVertex2f(xPos + charWidth*3/5, y);
                glVertex2f(xPos + charWidth*4/5, y + charHeight/3);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'x':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case 'y':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/4, y + charHeight/4);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth*3/4, y - charHeight/4);
                glEnd();
                break;
                
            case 'z':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            // Add more special characters before the default case
                
            case '!':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y + charHeight/6);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glEnd();
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y);
                glEnd();
                break;
                
            case '@':
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/4);
                glVertex2f(xPos + charWidth/2, y + charHeight/6);
                glVertex2f(xPos + charWidth/4, y + charHeight/4);
                glVertex2f(xPos + charWidth/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight*7/8);
                glVertex2f(xPos + charWidth*7/8, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*7/8, y + charHeight/4);
                glVertex2f(xPos + charWidth/2, y + charHeight/8);
                glEnd();
                break;
                
            case '#':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/3, y);
                glVertex2f(xPos + charWidth/3, y + charHeight);
                glVertex2f(xPos + charWidth*2/3, y);
                glVertex2f(xPos + charWidth*2/3, y + charHeight);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/3);
                glVertex2f(xPos, y + charHeight*2/3);
                glVertex2f(xPos + charWidth, y + charHeight*2/3);
                glEnd();
                break;
                
            case '$':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos, y + charHeight/4);
                glEnd();
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y - charHeight/8);
                glVertex2f(xPos + charWidth/2, y + charHeight + charHeight/8);
                glEnd();
                break;
                
            case '%':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth/6, y + charHeight*5/6);
                glVertex2f(xPos + charWidth/3, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight*5/6);
                glVertex2f(xPos + charWidth/3, y + charHeight*2/3);
                glEnd();
                glBegin(GL_LINE_LOOP);
                glVertex2f(xPos + charWidth*2/3, y + charHeight/3);
                glVertex2f(xPos + charWidth*5/6, y + charHeight/6);
                glVertex2f(xPos + charWidth*2/3, y);
                glVertex2f(xPos + charWidth/2, y + charHeight/6);
                glEnd();
                break;
                
            case '^':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight*2/3);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*2/3);
                glEnd();
                break;
                
            case '&':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*3/4, y);
                glVertex2f(xPos + charWidth/2, y + charHeight/3);
                glVertex2f(xPos + charWidth/4, y);
                glVertex2f(xPos, y + charHeight/3);
                glVertex2f(xPos + charWidth/2, y + charHeight*2/3);
                glVertex2f(xPos, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight*2/3);
                glEnd();
                break;
                
            case '*':
                glBegin(GL_LINES);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight/4);
                glEnd();
                break;
                
            case '?':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth, y + charHeight*3/4);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth/2, y + charHeight/4);
                glEnd();
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y);
                glEnd();
                break;
                
            case '<':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth, y);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth, y + charHeight);
                glEnd();
                break;
                
            case '>':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glVertex2f(xPos, y + charHeight);
                glEnd();
                break;
                
            case '{':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth*3/4, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight/4);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth*3/4, y);
                glEnd();
                break;
                
            case '}':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/4, y + charHeight);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glVertex2f(xPos + charWidth*3/4, y + charHeight*3/4);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/4);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/4, y);
                glEnd();
                break;
                
            case '|':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glEnd();
                break;
                
            case '~':
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos, y + charHeight/2);
                glVertex2f(xPos + charWidth/4, y + charHeight*2/3);
                glVertex2f(xPos + charWidth*3/4, y + charHeight/3);
                glVertex2f(xPos + charWidth, y + charHeight/2);
                glEnd();
                break;
                
            case '_':
                glBegin(GL_LINES);
                glVertex2f(xPos, y);
                glVertex2f(xPos + charWidth, y);
                glEnd();
                break;
                
            case ';':
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glEnd();
                glBegin(GL_LINE_STRIP);
                glVertex2f(xPos + charWidth/2, y + charHeight/5);
                glVertex2f(xPos + charWidth/3, y);
                glEnd();
                break;
                
            case '\'':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/2, y + charHeight*2/3);
                glVertex2f(xPos + charWidth/2, y + charHeight);
                glEnd();
                break;
                
            case '"':
                glBegin(GL_LINES);
                glVertex2f(xPos + charWidth/3, y + charHeight*2/3);
                glVertex2f(xPos + charWidth/3, y + charHeight);
                glVertex2f(xPos + charWidth*2/3, y + charHeight*2/3);
                glVertex2f(xPos + charWidth*2/3, y + charHeight);
                glEnd();
                break;
                
            // Default case - just draw a small dot for unimplemented characters
            default:
                glPointSize(3.0f);
                glBegin(GL_POINTS);
                glVertex2f(xPos + charWidth/2, y + charHeight/2);
                glEnd();
                glPointSize(1.0f);
                break;
        }
        
        // Advance cursor
        xPos += charWidth + spacing;
    }
    
    // Reset OpenGL state
    glLineWidth(1.0f);
    glPointSize(1.0f);
}

} // namespace Debug 