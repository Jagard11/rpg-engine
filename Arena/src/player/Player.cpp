#include "player/Player.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include "world/World.hpp"
#include <fstream>
#include <iostream>
#include "debug/VoxelDebug.hpp"

Player::Player()
    : m_position(0.0f, 102.0f, 0.0f)
    , m_velocity(0.0f)
    , m_forward(0.0f, 0.0f, 1.0f)
    , m_right(1.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_yaw(-90.0f)
    , m_pitch(0.0f)
    , m_moveSpeed(5.0f)
    , m_jumpForce(8.5f)
    , m_isOnGround(false)
    , m_isFlying(false)
    , m_jetpackEnabled(false)
    , m_jetpackFuel(100.0f)
    , m_canDoubleJump(false)
    , m_width(0.6f)  // Visual width - should match collision model's width
    , m_height(1.8f)
    , m_ignoreNextMouseMovement(false)
    , m_firstMouse(true)
    , m_lastX(0.0)
    , m_lastY(0.0)
{
    updateVectors();
    // Initialize the collision system
    m_collisionSystem.init(m_width, m_height);
    
    // Set a custom collision box that will allow the player to fall into 1x1 holes
    CollisionBox playerBox(0.25f, m_height, 0.25f);
    m_collisionSystem.setCollisionBox(playerBox);
}

Player::~Player() {
}

void Player::update(float deltaTime, World* world) {
    // Debug output for position - helps identify if player is falling through blocks
    static int debugCounter = 0;
    bool verboseUpdate = (debugCounter++ % 120 == 0); // Output every ~2 seconds at 60fps
    
    // Track if player is stuck at chunk boundaries
    static glm::vec3 lastPosition = m_position;
    static float stuckTime = 0.0f;
    static bool wasAtBoundary = false;
    
    // Check if player is at chunk boundary
    bool isAtBoundary = false;
    if (world) {
        // Check X boundaries - improve detection for exact chunk boundaries
        if (std::abs(std::fmod(m_position.x, World::CHUNK_SIZE)) < 0.05f || 
            std::abs(std::fmod(m_position.x, World::CHUNK_SIZE) - World::CHUNK_SIZE) < 0.05f ||
            std::abs(std::fmod(std::floor(m_position.x), World::CHUNK_SIZE)) < 0.001f) {
            isAtBoundary = true;
        }
        // Check Z boundaries - improve detection for exact chunk boundaries
        if (std::abs(std::fmod(m_position.z, World::CHUNK_SIZE)) < 0.05f || 
            std::abs(std::fmod(m_position.z, World::CHUNK_SIZE) - World::CHUNK_SIZE) < 0.05f ||
            std::abs(std::fmod(std::floor(m_position.z), World::CHUNK_SIZE)) < 0.001f) {
            isAtBoundary = true;
        }
        // Check Y boundaries - improve detection for exact chunk boundaries
        if (std::abs(std::fmod(m_position.y, World::CHUNK_HEIGHT)) < 0.05f || 
            std::abs(std::fmod(m_position.y, World::CHUNK_HEIGHT) - World::CHUNK_HEIGHT) < 0.05f ||
            std::abs(std::fmod(std::floor(m_position.y), World::CHUNK_HEIGHT)) < 0.001f) {
            isAtBoundary = true;
        }
        
        // CRITICAL FIX: Additional check for exact boundaries at x=16, z=16, etc.
        glm::ivec3 localPos = world->worldToLocalPos(m_position);
        if (localPos.x == 0 || localPos.x == World::CHUNK_SIZE - 1 ||
            localPos.z == 0 || localPos.z == World::CHUNK_SIZE - 1) {
            isAtBoundary = true;
        }
    }
    
    // Check if we've been stuck in the same position for a while
    if (isAtBoundary) {
        // Check if position hasn't changed much
        float positionDelta = glm::distance(m_position, lastPosition);
        if (positionDelta < 0.01f) {
            // At boundary and not moving
            stuckTime += deltaTime;
            
            // If we've been stuck for more than 0.5 seconds, attempt to adjust position
            if (stuckTime > 0.5f) {
                // Record trace for debugging
                std::string contextMessage = "Player stuck at (" + 
                    std::to_string(m_position.x) + "," + 
                    std::to_string(m_position.y) + "," + 
                    std::to_string(m_position.z) + ")";
                Debug::VoxelDebug::recordStackTrace(contextMessage);
                
                // Apply strong boundary adjustment
                m_collisionSystem.adjustPositionAtBlockBoundaries(m_position, true);
                
                // Try to move player away from the boundary with a larger nudge
                float nudgeX = 0, nudgeZ = 0;
                
                // CRITICAL FIX: Better detect which boundary we're on
                float xMod = std::fmod(m_position.x, World::CHUNK_SIZE);
                float zMod = std::fmod(m_position.z, World::CHUNK_SIZE);
                
                // Check for exact boundary values and apply appropriate nudge
                if (xMod < 0.05f)
                    nudgeX = 0.2f; // Increased from 0.15f
                else if (xMod > World::CHUNK_SIZE - 0.05f)
                    nudgeX = -0.2f; // Increased from -0.15f
                
                if (zMod < 0.05f)
                    nudgeZ = 0.2f; // Increased from 0.15f
                else if (zMod > World::CHUNK_SIZE - 0.05f)
                    nudgeZ = -0.2f; // Increased from -0.15f
                
                // Check for special case at exact boundary (like x=16.0)
                if (std::abs(std::fmod(m_position.x, World::CHUNK_SIZE)) < 0.001f && 
                    std::fmod(m_position.x, World::CHUNK_SIZE) > 0) {
                    nudgeX = 0.25f; // Stronger nudge for exact boundaries
                }
                
                if (std::abs(std::fmod(m_position.z, World::CHUNK_SIZE)) < 0.001f && 
                    std::fmod(m_position.z, World::CHUNK_SIZE) > 0) {
                    nudgeZ = 0.25f; // Stronger nudge for exact boundaries
                }
                
                // Apply the nudge
                m_position.x += nudgeX;
                m_position.z += nudgeZ;
                
                // CRITICAL FIX: If we're still in a solid block after nudging, try to move upward
                if (world->getBlock(glm::ivec3(m_position)) > 0) {
                    m_position.y += 0.5f;
                    std::cout << "Applied upward nudge to escape stuck position" << std::endl;
                }
                
                stuckTime = 0.0f; // Reset timer
            }
        } else {
            // Moving, but still at boundary
            stuckTime = 0.0f;
        }
        wasAtBoundary = true;
    } else {
        // Not at boundary anymore
        stuckTime = 0.0f;
        wasAtBoundary = false;
    }
    
    lastPosition = m_position;

    if (verboseUpdate) {
        std::cout << "===== PLAYER UPDATE =====" << std::endl;
        std::cout << "Player position: (" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")" << std::endl;
        std::cout << "Player velocity: (" << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << ")" << std::endl;
        std::cout << "Is on ground: " << (m_isOnGround ? "YES" : "NO") << std::endl;
        std::cout << "Is flying: " << (m_isFlying ? "YES" : "NO") << std::endl;
        std::cout << "Jetpack enabled: " << (m_jetpackEnabled ? "YES" : "NO") << std::endl;
        std::cout << "Jetpack fuel: " << m_jetpackFuel << std::endl;
        
        if (world) {
            int blockAtPlayerPos = world->getBlock(glm::ivec3(m_position.x, m_position.y, m_position.z));
            int blockBelowPlayer = world->getBlock(glm::ivec3(m_position.x, m_position.y - 0.1f, m_position.z));
            std::cout << "Block at player position: " << blockAtPlayerPos << " (0 = air)" << std::endl;
            std::cout << "Block below player: " << blockBelowPlayer << " (0 = air)" << std::endl;
        }
    }

    // Store old position and velocity for debugging
    glm::vec3 oldPosition = m_position;
    glm::vec3 oldVelocity = m_velocity;
    
    // Track the last known safe position (above ground)
    static glm::vec3 lastSafePosition = m_position;
    static float timeSinceLastSafePosition = 0.0f;
    
    // If player is currently above ground level and not falling at extreme velocity, update safe position
    if (m_position.y > 1.0f && (m_isOnGround || (m_velocity.y > -10.0f && m_position.y > 10.0f))) {
        lastSafePosition = m_position;
        timeSinceLastSafePosition = 0.0f;
    } else {
        timeSinceLastSafePosition += deltaTime;
    }
    
    // Failsafe - detect if player is falling through the world or is below y=0
    static float lastY = m_position.y;
    static float timeBelowGround = 0.0f;
    
    // CRITICAL FIX: Teleport player when they go below safe level (allow a small buffer below 0)
    if (m_position.y < -1.0f || timeBelowGround > 0.5f) {
        // If player has been below a safe level for too long or is definitely below ground
        std::cout << "Player fell below safe level (y=" << m_position.y 
                 << ", time below ground: " << timeBelowGround 
                 << "s), teleporting back up..." << std::endl;
        
        // Try to teleport to last safe position first
        if (timeSinceLastSafePosition < 5.0f && lastSafePosition.y > 5.0f) {
            m_position = lastSafePosition;
            std::cout << "Teleported to last safe position: (" 
                     << m_position.x << ", " << m_position.y << ", " << m_position.z << ")" << std::endl;
        } else {
            // Find a safe position using the collision system
            m_position = m_collisionSystem.findSafePosition(m_position, world, 100.0f);
            std::cout << "Teleported player to: (" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")" << std::endl;
        }
        
        // Reset values after teleportation
        m_velocity = glm::vec3(0.0f);
        m_isOnGround = false;
        timeBelowGround = 0.0f;
        return; // Skip the rest of the update
    } 
    else if (m_position.y < 0.5f) {
        // Track time spent at suspiciously low y values
        timeBelowGround += deltaTime;
    } else {
        timeBelowGround = 0.0f;
    }
    
    // Track extreme falling - if we fall more than 10 blocks in one frame, something is wrong
    float yDiff = lastY - m_position.y;
    if (yDiff > 10.0f && !m_isFlying) {
        std::cout << "Player fell " << yDiff << " blocks in one frame! Teleporting back..." << std::endl;
        m_position = oldPosition; // Use the position from the beginning of this frame
        m_velocity.y = 0.0f;
    }
    lastY = m_position.y;
    
    // Note: Block change detection moved to moveWithCollision method
    
    if (!m_isFlying) {
        // Apply gravity
        const float gravity = -20.0f;
        m_velocity.y += gravity * deltaTime;
        
        // Cap falling velocity
        if (m_velocity.y < -30.0f) {
            m_velocity.y = -30.0f;
        }
        
        // Store original position
        glm::vec3 originalPos = m_position;
        
        // Apply vertical movement with extra careful collision checks
        float stepSize = 0.02f; // Reduced from 0.05f to 0.02f for better precision
        float remainingMovement = m_velocity.y * deltaTime;
        m_isOnGround = false;
        
        // CHUNK BOUNDARY FIX: Check for ground using the collision system
        if (m_velocity.y <= 0) { // Only when falling or standing
            // Check for ground collision
            m_isOnGround = m_collisionSystem.checkGroundCollision(m_position, m_velocity, world);
            
            if (m_isOnGround) {
                m_velocity.y = 0;
                if (verboseUpdate) {
                    std::cout << "Ground detected beneath player, skipping downward movement" << std::endl;
                }
                remainingMovement = 0;
            }
        }
        
        // If we're not on ground and falling, do the careful stepped movement
        if (!m_isOnGround && m_velocity.y < 0) {
            // Move in small increments to catch all collisions
            int safetyCounter = 0; // Prevent infinite loops
            int collisionIterations = 0;
            bool edgeDetected = false;
            
            while (std::abs(remainingMovement) > 0.001f && safetyCounter++ < 1000) {
                // Use smaller steps when close to blocks or when moving quickly
                float step = (std::abs(remainingMovement) < stepSize) ? remainingMovement : (remainingMovement > 0 ? stepSize : -stepSize * 0.5f);
                
                // Apply step
                m_position.y += step;
                remainingMovement -= step;
                
                // CRITICAL FIX: After each step, check if we've gone below y=0
                if (m_position.y < 0.0f) {
                    m_position.y = originalPos.y; // Revert to original position
                    m_velocity.y = 0.0f; // Stop falling
                    m_isOnGround = true; // Consider player on ground
                    if (verboseUpdate) {
                        std::cout << "Prevented falling below y=0" << std::endl;
                    }
                    break;
                }
                
                // Check for collisions after step
                if (m_collisionSystem.collidesWithBlocks(m_position, m_velocity, world, step < 0)) {
                    collisionIterations++;
                    
                    // Check if we're hitting the ground or a wall/edge
                    bool groundCollision = m_collisionSystem.checkGroundCollision(m_position, m_velocity, world);
                    
                    if (groundCollision) {
                        // We've hit the ground - stop vertical movement
                        m_position.y -= step;
                        m_isOnGround = true;
                        m_velocity.y = 0;
                        
                        if (verboseUpdate) {
                            std::cout << "Ground collision detected" << std::endl;
                        }
                        break;
                    } else {
                        // We've hit a wall, not the ground
                        // First try to detect and handle vertical edges
                        glm::vec3 originalPos = m_position;
                        
                        // Try to correct vertical wall/edge collision
                        m_collisionSystem.correctVerticalWallCollision(m_position, m_velocity, world);
                        
                        // If position changed after correction, we've successfully handled an edge or wall
                        if (originalPos != m_position) {
                            edgeDetected = true;
                            
                            // Don't completely stop vertical movement when sliding down walls
                            // Instead, slow it down to simulate friction against the wall
                            if (m_velocity.y < 0) {
                                // Slow down vertical movement but don't stop it
                                m_velocity.y *= 0.85f; // 15% slowdown per collision
                                
                                if (verboseUpdate) {
                                    std::cout << "Wall collision - sliding down at velocity " << m_velocity.y << std::endl;
                                }
                            }
                            
                            // Continue movement after correction
                            continue;
                        } else {
                            // If correction didn't work, revert the step and stop movement
                            m_position.y -= step;
                            
                            // Only stop vertical movement if hitting ground, not wall
                            if (step < 0 && groundCollision) {
                                m_isOnGround = true;
                                m_velocity.y = 0;
                            } else if (step > 0) {
                                // If moving up, we hit a ceiling
                                m_velocity.y = 0;
                            }
                            
                            break;
                        }
                    }
                }
            }
            
            if (verboseUpdate) {
                if (collisionIterations > 0) {
                    std::cout << "Vertical movement collisions: " << collisionIterations << std::endl;
                }
                if (edgeDetected) {
                    std::cout << "Edge collision detected - applied edge handling" << std::endl;
                }
            }
        } else if (!m_isOnGround && m_velocity.y > 0) {
            // For upward movement, proceed with normal collision checks
            int safetyCounter = 0;
            int collisionIterations = 0;
            while (std::abs(remainingMovement) > 0.001f && safetyCounter++ < 1000) {
                float step = (std::abs(remainingMovement) < stepSize) ? remainingMovement : stepSize;
                
                // Apply step
                m_position.y += step;
                remainingMovement -= step;
                
                // Check collisions after step
                if (m_collisionSystem.collidesWithBlocks(m_position, m_velocity, world, false)) {
                    collisionIterations++;
                    // Revert step and stop movement
                    m_position.y -= step;
                    m_velocity.y = 0;
                    if (verboseUpdate) {
                        std::cout << "Collision detected during upward movement (hit ceiling)" << std::endl;
                    }
                    break;
                }
            }
            
            if (verboseUpdate && collisionIterations > 0) {
                std::cout << "Vertical movement collisions: " << collisionIterations << std::endl;
            }
        }
        
        // Reset double jump ability when on ground
        if (m_isOnGround) {
            m_canDoubleJump = true;
        }
    }
    
    // Always update the camera vectors
    updateVectors();
    
    // Log position and velocity changes for debugging
    if (verboseUpdate) {
        std::cout << "Position delta: (" 
                  << (m_position.x - oldPosition.x) << ", " 
                  << (m_position.y - oldPosition.y) << ", " 
                  << (m_position.z - oldPosition.z) << ")" << std::endl;
        
        std::cout << "Velocity delta: (" 
                  << (m_velocity.x - oldVelocity.x) << ", " 
                  << (m_velocity.y - oldVelocity.y) << ", " 
                  << (m_velocity.z - oldVelocity.z) << ")" << std::endl;
        
        std::cout << "Final position: (" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")" << std::endl;
        std::cout << "Final velocity: (" << m_velocity.x << ", " << m_velocity.y << ", " << m_velocity.z << ")" << std::endl;
        std::cout << "=========================" << std::endl;
    }
}

void Player::handleInput(float deltaTime, World* world) {
    // Handle keyboard movement
    glm::vec3 movement(0.0f);
    
    // Get input from GLFW (should be handled elsewhere, but simplifying for this example)
    GLFWwindow* window = glfwGetCurrentContext();
    
    // Forward/backward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        movement += m_forward * m_moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        movement -= m_forward * m_moveSpeed * deltaTime;
    }
    
    // Left/right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        movement -= m_right * m_moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        movement += m_right * m_moveSpeed * deltaTime;
    }
    
    // Toggle jetpack with X key - make sure this works properly
    static bool xKeyWasPressed = false;
    bool xKeyIsPressed = glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS;
    
    if (xKeyIsPressed && !xKeyWasPressed) {
        // Toggle jetpack state
        m_jetpackEnabled = !m_jetpackEnabled;
        
        // If turning off, disable flying mode too
        if (!m_jetpackEnabled) {
            m_isFlying = false;
        }
        
        std::cout << "Jetpack " << (m_jetpackEnabled ? "enabled" : "disabled") 
                  << " (Fuel: " << m_jetpackFuel << ")" << std::endl;
    }
    xKeyWasPressed = xKeyIsPressed;
    
    // Jump or fly up with Space
    bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    
    if (spacePressed) {
        if (m_jetpackEnabled && m_jetpackFuel > 0) {
            // Activate flying and move up
            m_isFlying = true;
            movement.y += m_moveSpeed * deltaTime * 1.5f;
            
            // Consume fuel
            m_jetpackFuel = std::max(0.0f, m_jetpackFuel - 15.0f * deltaTime);
            
            if (m_jetpackFuel <= 0) {
                std::cout << "Jetpack out of fuel!" << std::endl;
                m_isFlying = false;
            }
        } 
        else if (m_isOnGround) {
            // Normal jump when on ground
            jump();
        }
    }
    
    // Fly down with Shift when jetpack is enabled
    if (m_jetpackEnabled && m_jetpackFuel > 0 && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        m_isFlying = true;
        movement.y -= m_moveSpeed * deltaTime;
        
        // Consume less fuel when descending
        m_jetpackFuel = std::max(0.0f, m_jetpackFuel - 5.0f * deltaTime);
    }
    
    // Recharge jetpack when not flying
    if (!m_isFlying && m_jetpackFuel < 100.0f) {
        m_jetpackFuel = std::min(100.0f, m_jetpackFuel + 7.5f * deltaTime);
    }
    
    // Apply movement with collision detection
    if (m_isFlying) {
        // In flying mode, handle all movement vectors directly
        moveWithCollision(movement, world);
    } else {
        // In walking mode, split horizontal and vertical movement
        glm::vec3 horizontalMovement(movement.x, 0.0f, movement.z);
        moveWithCollision(horizontalMovement, world);
    }
    
    // Mouse input for camera rotation
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    
    if (m_ignoreNextMouseMovement) {
        // If this is the first movement after re-capturing the cursor,
        // just update the last position without changing the camera
        m_lastX = mouseX;
        m_lastY = mouseY;
        m_ignoreNextMouseMovement = false;
        return;
    }
    
    if (m_firstMouse) {
        m_lastX = mouseX;
        m_lastY = mouseY;
        m_firstMouse = false;
    }
    
    double xOffset = mouseX - m_lastX;
    double yOffset = m_lastY - mouseY;
    
    m_lastX = mouseX;
    m_lastY = mouseY;
    
    const float mouseSensitivity = 0.1f;
    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;
    
    m_yaw += xOffset;
    m_pitch += yOffset;
    
    // Clamp pitch to avoid flipping
    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;
    
    updateVectors();
}

void Player::updateVectors() {
    // Calculate the new forward vector
    m_forward.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_forward.y = sin(glm::radians(m_pitch));
    m_forward.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_forward = glm::normalize(m_forward);
    
    // Calculate right and up vectors
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
    
    // For movement, only flatten the vectors when not flying
    if (!m_isFlying) {
        glm::vec3 flatForward(m_forward.x, 0.0f, m_forward.z);
        flatForward = glm::normalize(flatForward);
        
        // Only use the flattened vector for movement, preserve the camera direction
        glm::vec3 movementForward = flatForward;
        
        glm::vec3 flatRight(m_right.x, 0.0f, m_right.z);
        flatRight = glm::normalize(flatRight);
        
        // Update movement vectors
        m_right = flatRight;
    }
}

void Player::jump() {
    if (m_isOnGround) {
        // Standard jump from ground
        m_velocity.y = m_jumpForce;
        m_isOnGround = false;
    } else if (m_jetpackEnabled && m_jetpackFuel > 0) {
        // Jetpack boost in mid-air
        m_velocity.y = std::max(m_velocity.y + 5.0f, 10.0f); // Increased from 8.0f to maintain advantage over regular jump
        
        // Consume fuel for the boost
        m_jetpackFuel = std::max(0.0f, m_jetpackFuel - 5.0f);
        
        // Activate flying mode
        m_isFlying = true;
        
        if (m_jetpackFuel <= 0) {
            std::cout << "Jetpack out of fuel!" << std::endl;
            m_isFlying = false;
        }
    } else if (!m_isOnGround && m_canDoubleJump) {
        // Double jump if enabled and available
        m_velocity.y = m_jumpForce * 0.85f; // Increased from 0.8f for better height on double jumps
        m_canDoubleJump = false; // Use up the double jump ability
    }
}

void Player::moveWithCollision(const glm::vec3& movement, World* world) {
    if (glm::length(movement) < 0.0001f) return;
    
    // Check if blocks beneath us have been modified before applying movement
    // This prevents the player from hovering when blocks are removed
    static float lastPhysicsCheckTime = 0.0f;
    float currentTime = glfwGetTime();
    
    // Only run this check periodically (max every 0.1 seconds) to avoid performance impact
    if (currentTime - lastPhysicsCheckTime > 0.1f && !m_isFlying && m_isOnGround) {
        bool blocksChangedBelowPlayer = world && world->checkPlayerPhysicsUpdate(m_position, m_width, m_height);
        if (blocksChangedBelowPlayer) {
            // Player is standing on a block that was just removed
            m_isOnGround = false;
            m_velocity.y = -0.1f; // Small downward velocity to initiate falling
            std::cout << "Block removed beneath player, initiating fall" << std::endl;
        }
        lastPhysicsCheckTime = currentTime;
    }
    
    // Use the collision system to handle movement
    m_position = m_collisionSystem.moveWithCollision(m_position, movement, m_velocity, world, m_isFlying, m_isOnGround);
    
    // Always check for and fix chunk boundary issues after movement
    // This helps prevent getting stuck even during normal movement
    if (world) {
        bool atChunkBoundaryX = std::abs(fmod(m_position.x, World::CHUNK_SIZE)) < 0.05f || 
                               std::abs(fmod(m_position.x, World::CHUNK_SIZE) - World::CHUNK_SIZE) < 0.05f;
        bool atChunkBoundaryZ = std::abs(fmod(m_position.z, World::CHUNK_SIZE)) < 0.05f || 
                               std::abs(fmod(m_position.z, World::CHUNK_SIZE) - World::CHUNK_SIZE) < 0.05f;
        
        if (atChunkBoundaryX || atChunkBoundaryZ) {
            // Apply a minor adjustment during normal movement to prevent sticking
            m_collisionSystem.adjustPositionAtBlockBoundaries(m_position, false);
        }
    }
}

glm::vec3 Player::getMinBounds() const {
    return m_collisionSystem.getMinBounds(m_position, m_forward);
}

glm::vec3 Player::getMaxBounds() const {
    return m_collisionSystem.getMaxBounds(m_position, m_forward);
}

bool Player::saveToFile(const std::string& filepath) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    // Write player data
    file.write(reinterpret_cast<const char*>(&m_position), sizeof(m_position));
    file.write(reinterpret_cast<const char*>(&m_yaw), sizeof(m_yaw));
    file.write(reinterpret_cast<const char*>(&m_pitch), sizeof(m_pitch));
    file.write(reinterpret_cast<const char*>(&m_isFlying), sizeof(m_isFlying));
    
    return file.good();
}

bool Player::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filepath << std::endl;
        return false;
    }
    
    // Read player data
    file.read(reinterpret_cast<char*>(&m_position), sizeof(m_position));
    file.read(reinterpret_cast<char*>(&m_yaw), sizeof(m_yaw));
    file.read(reinterpret_cast<char*>(&m_pitch), sizeof(m_pitch));
    file.read(reinterpret_cast<char*>(&m_isFlying), sizeof(m_isFlying));
    
    // Update derived values
    updateVectors();
    
    return file.good();
} 