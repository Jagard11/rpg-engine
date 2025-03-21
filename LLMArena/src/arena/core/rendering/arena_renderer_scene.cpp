// src/arena/core/rendering/arena_renderer_scene.cpp
#include "../../include/arena/core/rendering/arena_renderer.h"
#include "../include/arena/core/arena_core.h"
#include "../../include/arena/game/player_controller.h"
#include <QDebug>

void ArenaRenderer::setArenaParameters(double radius, double wallHeight) {
    // Debug the arena parameters
    qDebug() << "Setting arena parameters: radius =" << radius << "wallHeight =" << wallHeight;
    
    QString js = QString(
        "setArenaParameters(%1, %2);")
        .arg(radius)
        .arg(wallHeight);
    
    injectJavaScript(js);
    
    // Update game scene with new arena parameters
    gameScene->createOctagonalArena(radius, wallHeight);
}

void ArenaRenderer::updatePlayerPosition(double x, double y, double z) {
    if (!initialized) return;
    
    // Debug output to track player movement
    float rotation = playerController->getRotation();
    qDebug() << "Updating player camera: position:" << x << y << z << "rotation:" << rotation;
    
    QString js = QString(
        "updatePlayerPosition(%1, %2, %3, %4);")
        .arg(x)
        .arg(y)
        .arg(z)
        .arg(rotation);
    
    injectJavaScript(js);
    
    emit playerPositionUpdated(x, y, z);
}

// JavaScript for Three.js scene initialization
void ArenaRenderer::appendThreeJsSceneInit() {
    QString script = R"(
    // Initialize Three.js scene
    function init() {
        // Create scene
        scene = new THREE.Scene();
        scene.background = new THREE.Color(0x222222);
        
        // Create camera
        camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
        camera.position.set(0, 1.6, 0); // Default player height is 1.6 meters
        
        // Create renderer with appropriate settings for compatibility
        renderer = new THREE.WebGLRenderer({ 
            antialias: true, // Enable antialiasing for better quality
            precision: 'highp', // Use high precision for better rendering
            powerPreference: 'high-performance', // Prefer high performance mode
            alpha: false, // Disable alpha for better performance
            stencil: false, // Disable stencil for better performance
            depth: true, // Keep depth testing
            failIfMajorPerformanceCaveat: false // Don't fail on performance issues
        });
        renderer.setSize(window.innerWidth, window.innerHeight);
        document.getElementById('canvas-container').appendChild(renderer.domElement);
        
        // Set appropriate pixel ratio
        renderer.setPixelRatio(window.devicePixelRatio);
        
        // Add lights
        const ambientLight = new THREE.AmbientLight(0x404040);
        scene.add(ambientLight);
        
        const directionalLight = new THREE.DirectionalLight(0xffffff, 0.8);
        directionalLight.position.set(1, 1, 1);
        scene.add(directionalLight);
        
        // Create ground
        const groundGeometry = new THREE.CircleGeometry(arenaRadius, 32);
        const groundMaterial = new THREE.MeshBasicMaterial({ 
            color: 0x555555, 
            side: THREE.DoubleSide
        });
        const ground = new THREE.Mesh(groundGeometry, groundMaterial);
        ground.rotation.x = -Math.PI / 2;
        ground.position.y = -0.05; // Move ground slightly below 0 to avoid floor collisions
        scene.add(ground);
        
        // Add grid for better orientation
        const gridHelper = new THREE.GridHelper(arenaRadius * 2, 20, 0x444444, 0x333333);
        scene.add(gridHelper);
        
        // Create octagonal arena walls
        createArenaWalls(arenaRadius, wallHeight);
        
        // Handle window resize
        window.addEventListener('resize', onWindowResize);
        
        // Update debug info
        updateDebugInfo("Three.js initialized successfully");
    }
    
    // Create octagonal arena walls
    function createArenaWalls(radius, height) {
        // Create eight wall segments for octagon
        const wallMaterial = new THREE.MeshStandardMaterial({ 
            color: 0x888888,
            roughness: 0.7,
            metalness: 0.2
        });
        
        for (let i = 0; i < 8; i++) {
            const angle1 = Math.PI * 2 * i / 8;
            const angle2 = Math.PI * 2 * (i + 1) / 8;
            
            const x1 = radius * Math.cos(angle1);
            const z1 = radius * Math.sin(angle1);
            const x2 = radius * Math.cos(angle2);
            const z2 = radius * Math.sin(angle2);
            
            // Create wall geometry
            const wallWidth = Math.sqrt((x2-x1)**2 + (z2-z1)**2);
            const wallGeometry = new THREE.BoxGeometry(wallWidth, height, 0.2);
            
            const wall = new THREE.Mesh(wallGeometry, wallMaterial);
            
            // Position at midpoint of the wall segment
            const midX = (x1 + x2) / 2;
            const midZ = (z1 + z2) / 2;
            wall.position.set(midX, height/2, midZ);
            
            // Rotate to face center
            const angle = Math.atan2(midZ, midX);
            wall.rotation.y = angle + Math.PI/2;
            
            scene.add(wall);
            
            // Store wall in arena object for collision detection
            arena['wall' + i] = {
                mesh: wall,
                start: { x: x1, z: z1 },
                end: { x: x2, z: z2 }
            };
        }
    }
    
    // Handle window resize
    function onWindowResize() {
        if (useFallback) {
            // Resize fallback canvas
            if (fallbackCanvas) {
                const containerWidth = window.innerWidth - 40;
                const containerHeight = window.innerHeight - 100;
                const size = Math.min(containerWidth, containerHeight);
                
                fallbackCanvas.width = size;
                fallbackCanvas.height = size;
                
                // Re-render
                renderFallbackArena();
            }
            return;
        }
        
        // Resize 3D view
        if (camera && renderer) {
            camera.aspect = window.innerWidth / window.innerHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(window.innerWidth, window.innerHeight);
        }
    }
    
    // Animation loop
    function animate() {
        if (useFallback || !webGLAvailable) return;
        
        requestAnimationFrame(animate);
        
        if (renderer && scene && camera) {
            renderer.render(scene, camera);
        }
    }
    
    // JavaScript functions callable from C++
    function setArenaParameters(radius, wallHeight) {
        console.log(`Setting arena parameters: radius=${radius}, wallHeight=${wallHeight}`);
        
        // Update parameters for both modes
        arenaRadius = radius;
        wallHeight = wallHeight;
        
        if (useFallback) {
            // Update fallback visualization
            renderFallbackArena();
            return;
        }
        
        // 3D mode: remove existing arena
        for (let key in arena) {
            if (arena[key].mesh) {
                scene.remove(arena[key].mesh);
            }
        }
        arena = {};
        
        // Create new arena
        createArenaWalls(arenaRadius, wallHeight);
    }
    )";
    
    injectJavaScript(script);
}

// Function to inject animation updates for player movement
void ArenaRenderer::appendPlayerMovementCode() {
    QString script = R"(
    // Update player position and camera
    function updatePlayerPosition(x, y, z, rotation) {
        // Store previous values for comparison
        const oldX = player.x;
        const oldZ = player.z;
        const oldRotation = player.rotation;
        
        // Update player data
        player = {
            x: x,
            y: y,
            z: z,
            rotation: rotation
        };
        
        // Track if position actually changed
        const positionChanged = (oldX !== x || oldZ !== z || oldRotation !== rotation);
        
        if (useFallback) {
            // Update fallback visualization
            renderFallbackArena();
            return;
        }
        
        // Only update debug info every ~500ms to avoid flooding
        const now = Date.now();
        if (now - lastUpdateTime > 500) {
            lastUpdateTime = now;
            
            // Update debug display with current player position and rotation
            const debugMsg = `Player Position: (${x.toFixed(2)}, ${y.toFixed(2)}, ${z.toFixed(2)})\n` +
                            `Rotation: ${(rotation * 180 / Math.PI).toFixed(1)}° (${rotation.toFixed(2)} rad)`;
            updateDebugInfo(debugMsg);
        }
        
        // Update camera position and rotation for FPS view
        if (camera) {
            // Set camera position at player's eye level
            camera.position.set(x, y + 1.6, z);
            
            // Calculate look direction based on player rotation
            const lookX = x + Math.cos(rotation);
            const lookZ = z + Math.sin(rotation);
            
            // Set camera to look in the direction of player rotation
            camera.lookAt(lookX, y + 1.6, lookZ);
            
            // Log significant position changes
            if (positionChanged) {
                console.log(`Camera updated to: pos=(${x.toFixed(2)}, ${(y+1.6).toFixed(2)}, ${z.toFixed(2)}), ` +
                          `looking at (${lookX.toFixed(2)}, ${(y+1.6).toFixed(2)}, ${lookZ.toFixed(2)})`);
            }
        }
    }
    )";
    
    injectJavaScript(script);
}