// src/arena_renderer.cpp - Modified to improve OpenGL detection and support
#include "../include/arena_renderer.h"
#include "../include/game_scene.h"
#include "../include/player_controller.h"
#include <QFile>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebEngineProfile> // Added this include for QWebEngineProfile
#include <QMessageBox>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <stdexcept>

// Helper function to check if WebGL is supported with more detailed diagnostics
bool isWebGLSupported() {
    // Create QOffscreenSurface
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 0); // OpenGL 2.0 is required for WebGL
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setRenderableType(QSurfaceFormat::OpenGL); // Explicitly request OpenGL
    
    QOffscreenSurface surface;
    surface.setFormat(format);
    surface.create();
    if (!surface.isValid()) {
        qWarning() << "Failed to create valid offscreen surface for WebGL check";
        return false;
    }
    
    // Create OpenGL context
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create()) {
        qWarning() << "Failed to create OpenGL context for WebGL check";
        return false;
    }
    
    // Make it current
    if (!context.makeCurrent(&surface)) {
        qWarning() << "Failed to make OpenGL context current for WebGL check";
        return false;
    }
    
    // Check if context is valid and get OpenGL version
    bool isValid = context.isValid();
    QSurfaceFormat curFormat = context.format();
    int majorVersion = curFormat.majorVersion();
    int minorVersion = curFormat.minorVersion();
    
    // Get vendor and renderer strings
    QOpenGLFunctions *f = context.functions();
    QString vendor = QString::fromLatin1(reinterpret_cast<const char*>(f->glGetString(GL_VENDOR)));
    QString renderer = QString::fromLatin1(reinterpret_cast<const char*>(f->glGetString(GL_RENDERER)));
    QString version = QString::fromLatin1(reinterpret_cast<const char*>(f->glGetString(GL_VERSION)));
    
    qDebug() << "OpenGL context valid:" << isValid;
    qDebug() << "OpenGL version:" << majorVersion << "." << minorVersion;
    qDebug() << "OpenGL vendor:" << vendor;
    qDebug() << "OpenGL renderer:" << renderer;
    qDebug() << "OpenGL version string:" << version;
    
    // Check if we're using software rendering
    bool isSoftwareRenderer = renderer.contains("llvmpipe", Qt::CaseInsensitive) || 
                             renderer.contains("software", Qt::CaseInsensitive) ||
                             renderer.contains("swrast", Qt::CaseInsensitive);
    
    if (isSoftwareRenderer) {
        qWarning() << "Software rendering detected, hardware acceleration may not be available";
    }
    
    // WebGL requires at least OpenGL 2.0
    bool hasWebGL = isValid && (majorVersion > 2 || (majorVersion == 2 && minorVersion >= 0));
    
    context.doneCurrent();
    return hasWebGL;
}

ArenaRenderer::ArenaRenderer(QWidget *parent, CharacterManager *charManager) 
    : QObject(parent), initialized(false), characterManager(charManager) {
    
    // Check if WebGL is supported before initializing
    if (!isWebGLSupported()) {
        qWarning() << "WebGL is not supported on this system";
        throw std::runtime_error("WebGL not supported");
    }
    
    // Initialize web view for WebGL rendering
    webView = new QWebEngineView(qobject_cast<QWidget*>(parent));
    
    // Enable WebGL and hardware acceleration
    webView->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    webView->settings()->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    webView->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    
    // Enable developer tools for debugging if needed
    webView->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    webView->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    
    // Enable hardware rendering - no longer forcing software rendering
    webView->page()->settings()->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    
    // Set additional WebEngine settings to prefer hardware acceleration
    webView->page()->profile()->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    
    // Connect signals for page loading
    connect(webView, &QWebEngineView::loadFinished, this, &ArenaRenderer::handleLoadFinished);
    
    // Create the game scene
    gameScene = new GameScene(this);
    
    // Create the player controller
    playerController = new PlayerController(gameScene, this);
    
    // Set up the web channel for JavaScript communication
    webChannel = new QWebChannel(this);
    webChannel->registerObject("arenaRenderer", this);
    webView->page()->setWebChannel(webChannel);
}

ArenaRenderer::~ArenaRenderer() {
    // Clean up resources
    delete webView;
}

void ArenaRenderer::initialize() {
    // Load the HTML file for WebGL rendering
    QString htmlPath = QDir::currentPath() + "/resources/arena.html";
    QFile htmlFile(htmlPath);
    
    if (htmlFile.exists()) {
        qDebug() << "Loading existing arena HTML file from:" << htmlPath;
        webView->load(QUrl::fromLocalFile(htmlPath));
    } else {
        qDebug() << "Creating new arena HTML file at:" << htmlPath;
        
        // Create resources directory if it doesn't exist
        QDir dir;
        dir.mkpath(QDir::currentPath() + "/resources");
        
        // If file doesn't exist, create it with basic WebGL setup and fallback
        QString htmlContent = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>RPG Arena</title>
    <style>
        body { margin: 0; overflow: hidden; font-family: Arial, sans-serif; }
        canvas { display: block; }
        #error-container { 
            display: none; 
            position: absolute; 
            top: 0; 
            left: 0; 
            width: 100%; 
            height: 100%; 
            background-color: rgba(0,0,0,0.8);
            color: white;
            text-align: center;
            padding-top: 20%;
        }
        #canvas-container {
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
        }
        #fallback-container {
            display: none;
            position: absolute;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background-color: #222;
            color: white;
        }
        #fallback-canvas {
            background-color: #333;
            margin: 20px;
            border: 2px solid #555;
        }
        #fallback-info {
            position: absolute;
            bottom: 10px;
            left: 10px;
            background-color: rgba(0,0,0,0.7);
            padding: 10px;
            border-radius: 5px;
            font-size: 12px;
        }
        #fallback-title {
            margin-top: 10px;
            text-align: center;
        }
        #debug-info {
            position: absolute;
            top: 5px;
            right: 5px;
            background-color: rgba(0,0,0,0.5);
            color: white;
            padding: 5px;
            font-size: 10px;
            z-index: 100;
        }
    </style>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
</head>
<body>
    <div id="canvas-container"></div>
    <div id="debug-info"></div>
    
    <div id="error-container">
        <h2>WebGL Not Available</h2>
        <p>Your browser or system does not support WebGL or 3D acceleration.</p>
        <p>Please check your graphics drivers and try again.</p>
    </div>
    
    <div id="fallback-container">
        <h3 id="fallback-title">Top-down 2D View (Fallback Mode)</h3>
        <canvas id="fallback-canvas"></canvas>
        <div id="fallback-info">
            Using 2D fallback visualization (WebGL not available)<br>
            ⬤ Player | ■ Characters | ○ Arena boundary
        </div>
    </div>
    
    <script>
        let scene, camera, renderer;
        let arena = {};
        let characters = {};
        let player = {};
        let arenaRadius = 10;
        let wallHeight = 2;
        let arenaRenderer;
        let webGLAvailable = true;
        let useFallback = false;
        let fallbackCanvas, fallbackCtx;
        let debugInfo = document.getElementById('debug-info');

        // Enhanced WebGL detection with detailed logging
        function checkWebGL() {
            try {
                const canvas = document.createElement('canvas');
                const gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
                
                if (!gl) {
                    console.error("WebGL not available");
                    updateDebugInfo("WebGL not available");
                    return false;
                }
                
                // Get WebGL info
                const debugInfo = gl.getExtension('WEBGL_debug_renderer_info');
                let vendor = gl.getParameter(gl.VENDOR);
                let renderer = gl.getParameter(gl.RENDERER);
                
                if (debugInfo) {
                    vendor = gl.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);
                    renderer = gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);
                }
                
                const version = gl.getParameter(gl.VERSION);
                const glslVersion = gl.getParameter(gl.SHADING_LANGUAGE_VERSION);
                const extensions = gl.getSupportedExtensions();
                
                console.log("WebGL Vendor:", vendor);
                console.log("WebGL Renderer:", renderer);
                console.log("WebGL Version:", version);
                console.log("GLSL Version:", glslVersion);
                console.log("WebGL Extensions:", extensions);
                
                updateDebugInfo(`WebGL: ${vendor} - ${renderer}`);
                
                // Check if using software rendering
                const isSoftware = renderer.includes('SwiftShader') || 
                                 renderer.includes('llvmpipe') || 
                                 renderer.includes('Software') ||
                                 renderer.includes('swrast');
                
                if (isSoftware) {
                    console.warn("Software rendering detected");
                    updateDebugInfo(`WebGL: Software rendering (${renderer})`);
                }
                
                return true;
            } catch(e) {
                console.error("WebGL detection failed:", e);
                updateDebugInfo("WebGL detection error: " + e.message);
                return false;
            }
        }
        
        // Update debug info display
        function updateDebugInfo(message) {
            if (debugInfo) {
                debugInfo.textContent = message;
            }
        }
        
        // Fallback to basic canvas rendering when WebGL isn't available
        function initFallback() {
            console.log("Initializing fallback visualization");
            
            document.getElementById('fallback-container').style.display = 'block';
            document.getElementById('canvas-container').style.display = 'none';
            
            // Set up the 2D canvas for fallback rendering
            fallbackCanvas = document.getElementById('fallback-canvas');
            
            // Adjust canvas size based on window size
            const containerWidth = window.innerWidth - 40; // Account for margins
            const containerHeight = window.innerHeight - 100; // Account for header and info
            const size = Math.min(containerWidth, containerHeight);
            
            fallbackCanvas.width = size;
            fallbackCanvas.height = size;
            fallbackCtx = fallbackCanvas.getContext('2d');
            
            // Initial render of the arena
            renderFallbackArena();
            
            // Notify C++ that we're using fallback mode
            if (arenaRenderer) {
                arenaRenderer.handleJavaScriptMessage("Using fallback visualization mode");
            }
            
            useFallback = true;
        }
        
        // Render the 2D fallback arena and entities
        function renderFallbackArena() {
            if (!fallbackCtx) return;
            
            const canvas = fallbackCanvas;
            const ctx = fallbackCtx;
            const scale = canvas.width / (arenaRadius * 2.2); // Scale to fit with some margin
            
            // Clear canvas
            ctx.fillStyle = '#333';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            // Draw arena boundary (octagon)
            ctx.strokeStyle = '#777';
            ctx.lineWidth = 2;
            ctx.beginPath();
            
            for (let i = 0; i < 8; i++) {
                const angle = Math.PI * 2 * i / 8;
                const x = canvas.width / 2 + Math.cos(angle) * arenaRadius * scale;
                const y = canvas.height / 2 + Math.sin(angle) * arenaRadius * scale;
                
                if (i === 0) {
                    ctx.moveTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            }
            
            ctx.closePath();
            ctx.stroke();
            
            // Draw grid
            ctx.strokeStyle = '#444';
            ctx.lineWidth = 1;
            
            // Draw center lines
            ctx.beginPath();
            ctx.moveTo(canvas.width / 2, 0);
            ctx.lineTo(canvas.width / 2, canvas.height);
            ctx.moveTo(0, canvas.height / 2);
            ctx.lineTo(canvas.width, canvas.height / 2);
            ctx.stroke();
            
            // Draw characters
            for (let name in characters) {
                const char = characters[name];
                
                // Convert world coordinates to canvas coordinates
                const x = canvas.width / 2 + char.x * scale;
                const y = canvas.height / 2 + char.z * scale;
                
                // Draw rectangle for character
                if (char.missingTexture) {
                    // Hot pink for missing textures
                    ctx.fillStyle = '#FF00FF';
                } else {
                    // Normal character color
                    ctx.fillStyle = '#4CAF50';
                }
                
                const size = Math.max(char.width, char.depth) * scale;
                ctx.fillRect(x - size/2, y - size/2, size, size);
                
                // Draw character name
                ctx.fillStyle = 'white';
                ctx.font = '10px Arial';
                ctx.textAlign = 'center';
                ctx.fillText(name, x, y - size/2 - 5);
            }
            
            // Draw player
            if (player.x !== undefined) {
                const x = canvas.width / 2 + player.x * scale;
                const y = canvas.height / 2 + player.z * scale;
                
                // Draw circle for player
                ctx.fillStyle = '#FFC107';
                ctx.beginPath();
                ctx.arc(x, y, 8, 0, Math.PI * 2);
                ctx.fill();
                
                // Draw direction indicator
                ctx.strokeStyle = '#FFC107';
                ctx.lineWidth = 2;
                ctx.beginPath();
                ctx.moveTo(x, y);
                ctx.lineTo(
                    x + Math.cos(player.rotation) * 15, 
                    y + Math.sin(player.rotation) * 15
                );
                ctx.stroke();
                
                // Label
                ctx.fillStyle = 'white';
                ctx.font = '10px Arial';
                ctx.textAlign = 'center';
                ctx.fillText('Player', x, y - 15);
            }
        }

        // Initialize WebGL when document is loaded
        document.addEventListener('DOMContentLoaded', function() {
            // Set up Qt web channel
            new QWebChannel(qt.webChannelTransport, function(channel) {
                arenaRenderer = channel.objects.arenaRenderer;
                console.log("Web channel initialized");
                
                if (!checkWebGL()) {
                    document.getElementById('error-container').style.display = 'block';
                    console.error("WebGL not available");
                    
                    // Use fallback mode instead
                    initFallback();
                    
                    if (arenaRenderer) {
                        arenaRenderer.handleJavaScriptMessage("WebGL not available on this system, using fallback");
                    }
                    return;
                }
                
                // Initialize WebGL scene
                try {
                    init();
                    animate();
                    
                    // Notify C++ that initialization is complete
                    if (arenaRenderer) {
                        arenaRenderer.handleJavaScriptMessage("WebGL initialized successfully");
                    }
                } catch (e) {
                    console.error("WebGL initialization failed:", e);
                    document.getElementById('error-container').style.display = 'block';
                    
                    // Use fallback mode
                    initFallback();
                    
                    if (arenaRenderer) {
                        arenaRenderer.handleJavaScriptMessage("WebGL initialization failed: " + e.message + ", using fallback");
                    }
                }
            });
        });

        // Initialize Three.js scene
        function init() {
            // Create scene
            scene = new THREE.Scene();
            scene.background = new THREE.Color(0x222222);
            
            // Create camera
            camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
            camera.position.set(0, 1.6, 5); // Default player height is 1.6 meters
            
            // Create renderer with appropriate settings for compatibility
            // This is a key change - using high-performance settings instead of forcing software rendering
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
            const groundGeometry = new THREE.CircleGeometry(arenaRadius, 8);
            const groundMaterial = new THREE.MeshBasicMaterial({ 
                color: 0x555555, 
                side: THREE.DoubleSide
            });
            const ground = new THREE.Mesh(groundGeometry, groundMaterial);
            ground.rotation.x = -Math.PI / 2;
            scene.add(ground);
            
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
            const wallMaterial = new THREE.MeshBasicMaterial({ color: 0x888888 });
            
            for (let i = 0; i < 8; i++) {
                const angle1 = Math.PI * 2 * i / 8;
                const angle2 = Math.PI * 2 * (i + 1) / 8;
                
                const x1 = radius * Math.cos(angle1);
                const z1 = radius * Math.sin(angle1);
                const x2 = radius * Math.cos(angle2);
                const z2 = radius * Math.sin(angle2);
                
                // Create wall geometry from points
                const wallShape = new THREE.Shape();
                wallShape.moveTo(0, 0);
                wallShape.lineTo(0, height);
                wallShape.lineTo(Math.sqrt((x2-x1)**2 + (z2-z1)**2), height);
                wallShape.lineTo(Math.sqrt((x2-x1)**2 + (z2-z1)**2), 0);
                wallShape.lineTo(0, 0);
                
                const wallGeometry = new THREE.ExtrudeGeometry(wallShape, {
                    steps: 1,
                    depth: 0.1,
                    bevelEnabled: false
                });
                
                const wall = new THREE.Mesh(wallGeometry, wallMaterial);
                
                // Position and rotate the wall
                wall.position.set(x1, 0, z1);
                wall.lookAt(x2, 0, z2);
                
                scene.add(wall);
                
                // Store wall in arena object for collision detection
                arena['wall' + i] = {
                    mesh: wall,
                    start: { x: x1, z: z1 },
                    end: { x: x2, z: z2 }
                };
            }
        }
        
        // Create a billboard sprite for a character
        function createCharacterBillboard(characterName, spritePath, width, height, depth) {
            // Check if character already exists and clean up if needed
            if (characters[characterName]) {
                if (!useFallback && characters[characterName].sprite) {
                    scene.remove(characters[characterName].sprite);
                    scene.remove(characters[characterName].collisionBox);
                }
                delete characters[characterName];
            }
            
            if (useFallback) {
                console.log(`Created fallback character ${characterName}`);
                
                // Create a simple 2D representation for fallback mode
                characters[characterName] = {
                    x: 0,
                    y: 0,
                    z: 0,
                    width: width,
                    height: height,
                    depth: depth,
                    missingTexture: !spritePath || spritePath === ""
                };
                
                // Render the fallback view
                renderFallbackArena();
                return;
            }
            
            // Load texture for sprite
            const textureLoader = new THREE.TextureLoader();
            let missingTexture = false;
            
            // Use default texture if path is missing
            if (!spritePath || spritePath === "") {
                missingTexture = true;
                
                // Create a neon pink texture for missing sprites
                const canvas = document.createElement('canvas');
                canvas.width = 128;
                canvas.height = 256;
                const ctx = canvas.getContext('2d');
                
                // Fill with neon pink
                ctx.fillStyle = '#FF00FF';
                ctx.fillRect(0, 0, canvas.width, canvas.height);
                
                // Add text to indicate missing texture
                ctx.fillStyle = 'white';
                ctx.font = '20px Arial';
                ctx.textAlign = 'center';
                ctx.fillText('MISSING', canvas.width/2, canvas.height/2 - 10);
                ctx.fillText('TEXTURE', canvas.width/2, canvas.height/2 + 20);
                
                const texture = new THREE.CanvasTexture(canvas);
                createSpriteWithTexture(texture);
            } else {
                // Load normal texture from file
                textureLoader.load(
                    spritePath, 
                    function(texture) {
                        console.log("Sprite loaded: " + spritePath);
                        createSpriteWithTexture(texture);
                    },
                    undefined, // onProgress callback
                    function(error) {
                        console.error("Error loading texture: " + error);
                        
                        // Create a neon pink texture for error
                        const canvas = document.createElement('canvas');
                        canvas.width = 128;
                        canvas.height = 256;
                        const ctx = canvas.getContext('2d');
                        
                        // Fill with neon pink
                        ctx.fillStyle = '#FF00FF';
                        ctx.fillRect(0, 0, canvas.width, canvas.height);
                        
                        // Add text to indicate error
                        ctx.fillStyle = 'white';
                        ctx.font = '20px Arial';
                        ctx.textAlign = 'center';
                        ctx.fillText('TEXTURE', canvas.width/2, canvas.height/2 - 10);
                        ctx.fillText('ERROR', canvas.width/2, canvas.height/2 + 20);
                        
                        const texture = new THREE.CanvasTexture(canvas);
                        createSpriteWithTexture(texture);
                    }
                );
            }
            
            function createSpriteWithTexture(texture) {
                // Create sprite material
                const spriteMaterial = new THREE.SpriteMaterial({ 
                    map: texture,
                    transparent: true
                });
                
                // Create sprite
                const sprite = new THREE.Sprite(spriteMaterial);
                sprite.scale.set(width, height, 1);
                sprite.position.set(0, height/2, 0); // Center position in arena
                scene.add(sprite);
                
                // Create invisible collision box
                const boxGeometry = new THREE.BoxGeometry(width, height, depth);
                const boxMaterial = new THREE.MeshBasicMaterial({ 
                    transparent: true, 
                    opacity: 0.0, // Invisible
                    wireframe: true // Optional: make wireframe for debugging
                });
                
                const collisionBox = new THREE.Mesh(boxGeometry, boxMaterial);
                collisionBox.position.set(0, height/2, 0);
                scene.add(collisionBox);
                
                // Store character data
                characters[characterName] = {
                    sprite: sprite,
                    collisionBox: collisionBox,
                    width: width,
                    height: height,
                    depth: depth,
                    x: 0,
                    y: 0,
                    z: 0,
                    missingTexture: missingTexture
                };
                
                console.log(`Created character ${characterName} with dimensions: ${width}x${height}x${depth}`);
            }
        }
        
        // Update character position
        function updateCharacterPosition(characterName, x, y, z) {
            if (!characters[characterName]) return;
            
            // Store position data for both 3D and fallback modes
            characters[characterName].x = x;
            characters[characterName].y = y;
            characters[characterName].z = z;
            
            if (useFallback) {
                // Update fallback visualization
                renderFallbackArena();
                return;
            }
            
            // Update 3D objects
            if (characters[characterName].sprite) {
                characters[characterName].sprite.position.set(x, y + characters[characterName].height/2, z);
                characters[characterName].collisionBox.position.set(x, y + characters[characterName].height/2, z);
            }
        }
        
        // Update player position and camera
        function updatePlayerPosition(x, y, z, rotation) {
            // Store player data for both modes
            player = {
                x: x,
                y: y,
                z: z,
                rotation: rotation
            };
            
            if (useFallback) {
                // Update fallback visualization
                renderFallbackArena();
                return;
            }
            
            // Update 3D camera
            camera.position.set(x, y + 1.6, z); // Player eye height at 1.6m
            camera.rotation.y = rotation;
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
            camera.aspect = window.innerWidth / window.innerHeight;
            camera.updateProjectionMatrix();
            renderer.setSize(window.innerWidth, window.innerHeight);
        }
        
        // Animation loop
        function animate() {
            if (useFallback || !webGLAvailable) return;
            
            requestAnimationFrame(animate);
            
            // Update billboards to face camera
            for (let name in characters) {
                if (characters[name].sprite) {
                    // Billboard always faces camera
                    const sprite = characters[name].sprite;
                    sprite.material.rotation = camera.rotation.y;
                }
            }
            
            renderer.render(scene, camera);
        }
        
        // JavaScript functions callable from C++
        function setArenaParameters(radius, wallHeight) {
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
                scene.remove(arena[key].mesh);
            }
            arena = {};
            
            // Create new arena
            createArenaWalls(arenaRadius, wallHeight);
        }
        
        // Handle window resize events
        window.addEventListener('resize', onWindowResize);
    </script>
</body>
</html>
        )";
        
        // Write the HTML file
        if (htmlFile.open(QIODevice::WriteOnly)) {
            htmlFile.write(htmlContent.toUtf8());
            htmlFile.close();
            
            // Load the created file
            webView->load(QUrl::fromLocalFile(htmlPath));
        } else {
            qWarning() << "Failed to create HTML file at:" << htmlPath;
            throw std::runtime_error("Failed to create WebGL arena HTML file");
        }
    }
}

void ArenaRenderer::handleLoadFinished(bool ok) {
    if (ok) {
        qDebug() << "WebGL page loaded successfully";
        
        // Run diagnostic script to get WebGL details
        webView->page()->runJavaScript(
            "function getWebGLInfo() {"
            "  try {"
            "    const canvas = document.createElement('canvas');"
            "    const gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');"
            "    if (!gl) return 'WebGL not supported';"
            "    const debugInfo = gl.getExtension('WEBGL_debug_renderer_info');"
            "    let vendor = gl.getParameter(gl.VENDOR);"
            "    let renderer = gl.getParameter(gl.RENDERER);"
            "    if (debugInfo) {"
            "      vendor = gl.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);"
            "      renderer = gl.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);"
            "    }"
            "    return 'WebGL: ' + vendor + ' - ' + renderer;"
            "  } catch(e) {"
            "    return 'Error getting WebGL info: ' + e.message;"
            "  }"
            "}"
            "getWebGLInfo();",
            [this](const QVariant &result) {
                qDebug() << "WebGL Info:" << result.toString();
            }
        );
        
        // Initialize WebGL context
        initializeWebGL();
        
        // Set default arena parameters
        setArenaParameters(10.0, 2.0);
        
        // Create the player entity
        playerController->createPlayerEntity();
        
        // Start player controller updates
        playerController->startUpdates();
        
        initialized = true;
        emit renderingInitialized();
    } else {
        qDebug() << "Failed to load WebGL page";
        
        // Try to notify the user about the error
        emit renderingInitialized(); // Still emit signal so UI updates
    }
}

void ArenaRenderer::initializeWebGL() {
    // Execute JavaScript setup code
    injectJavaScript(R"(
        console.log("WebGL initialization from C++");
        
        // Check if fallback mode is active
        if (typeof useFallback !== 'undefined' && useFallback) {
            console.log("Using fallback visualization mode");
        }
        
        // Report WebGL capabilities
        if (typeof checkWebGL === 'function') {
            let webglSupport = checkWebGL();
            console.log("WebGL support: " + webglSupport);
        }
    )");
}

void ArenaRenderer::handleJavaScriptMessage(const QString &message) {
    qDebug() << "JavaScript message: " << message;
}

void ArenaRenderer::injectJavaScript(const QString &script) {
    webView->page()->runJavaScript(script, [](const QVariant &result) {
        // Optional callback for script execution result
    });
}

void ArenaRenderer::setActiveCharacter(const QString &name) {
    activeCharacter = name;
}

void ArenaRenderer::loadCharacterSprite(const QString &characterName, const QString &spritePath) {
    if (!initialized) {
        qDebug() << "Cannot load sprite, renderer not initialized";
        return;
    }
    
    qDebug() << "Loading character sprite:" << characterName << "path:" << spritePath;
    
    // Get character collision geometry
    CharacterCollisionGeometry geometry;
    
    // Use the passed CharacterManager if available
    if (characterManager) {
        try {
            CharacterAppearance appearance = characterManager->loadCharacterAppearance(characterName);
            geometry = appearance.collision;
        } catch (const std::exception& e) {
            qWarning() << "Error loading character appearance:" << e.what();
            // Use default geometry if there's an error
            geometry.width = 1.0;
            geometry.height = 2.0;
            geometry.depth = 1.0;
        }
    } else {
        // Use default geometry if we don't have a CharacterManager
        geometry.width = 1.0;
        geometry.height = 2.0;
        geometry.depth = 1.0;
    }
    
    // Create the character billboard in WebGL or fallback
    QString js;
    
    // If spritePath is empty, we'll use the missing texture indicator
    if (spritePath.isEmpty()) {
        js = QString(
            "createCharacterBillboard('%1', '', %2, %3, %4);")
            .arg(characterName)
            .arg(geometry.width)
            .arg(geometry.height)
            .arg(geometry.depth);
    } else {
        // Check if sprite file exists
        QFile spriteFile(spritePath);
        if (!spriteFile.exists()) {
            qWarning() << "Sprite file does not exist:" << spritePath;
            js = QString(
                "createCharacterBillboard('%1', '', %2, %3, %4);")
                .arg(characterName)
                .arg(geometry.width)
                .arg(geometry.height)
                .arg(geometry.depth);
        } else {
            js = QString(
                "createCharacterBillboard('%1', '%2', %3, %4, %5);")
                .arg(characterName)
                .arg(spritePath)
                .arg(geometry.width)
                .arg(geometry.height)
                .arg(geometry.depth);
        }
    }
    
    qDebug() << "Injecting JS for character billboard";
    injectJavaScript(js);
    
    // Place character in the center of the arena
    qDebug() << "Updating character position";
    updateCharacterPosition(characterName, 0, 0, 0);
}

void ArenaRenderer::updateCharacterPosition(const QString &characterName, double x, double y, double z) {
    if (!initialized) return;
    
    QString js = QString(
        "updateCharacterPosition('%1', %2, %3, %4);")
        .arg(characterName)
        .arg(x)
        .arg(y)
        .arg(z);
    
    injectJavaScript(js);
    
    emit characterPositionUpdated(characterName, x, y, z);
}

void ArenaRenderer::updatePlayerPosition(double x, double y, double z) {
    if (!initialized) return;
    
    float rotation = playerController->getRotation();
    
    QString js = QString(
        "updatePlayerPosition(%1, %2, %3, %4);")
        .arg(x)
        .arg(y)
        .arg(z)
        .arg(rotation);
    
    injectJavaScript(js);
    
    emit playerPositionUpdated(x, y, z);
}

void ArenaRenderer::setArenaParameters(double radius, double wallHeight) {
    if (!initialized) {
        // Save parameters to apply later when initialized
        gameScene->createOctagonalArena(radius, wallHeight);
        return;
    }
    
    QString js = QString(
        "setArenaParameters(%1, %2);")
        .arg(radius)
        .arg(wallHeight);
    
    injectJavaScript(js);
    
    // Update game scene with new arena parameters
    gameScene->createOctagonalArena(radius, wallHeight);
}

void ArenaRenderer::createArena(double radius, double wallHeight) {
    setArenaParameters(radius, wallHeight);
}

void ArenaRenderer::createCharacterBillboard(const QString &characterName, const QString &spritePath, 
                                          const CharacterCollisionGeometry &collisionGeometry) {
    loadCharacterSprite(characterName, spritePath);
}