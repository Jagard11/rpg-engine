// src/arena/core/rendering/arena_renderer_webgl.cpp
#include "../../../../include/arena/core/rendering/arena_renderer.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QFile>
#include <QDebug>

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
        return false;
    }
    
    // Create OpenGL context
    QOpenGLContext context;
    context.setFormat(format);
    if (!context.create()) {
        return false;
    }
    
    // Make it current
    if (!context.makeCurrent(&surface)) {
        return false;
    }
    
    // Check if context is valid and get OpenGL version
    bool isValid = context.isValid();
    QSurfaceFormat curFormat = context.format();
    int majorVersion = curFormat.majorVersion();
    int minorVersion = curFormat.minorVersion();
    
    // WebGL requires at least OpenGL 2.0
    bool hasWebGL = isValid && (majorVersion > 2 || (majorVersion == 2 && minorVersion >= 0));
    
    context.doneCurrent();
    return hasWebGL;
}

void ArenaRenderer::initializeWebGL() {
    // Execute JavaScript setup code
    injectJavaScript(R"(
        console.log("WebGL initialization started");
    )");
}

void ArenaRenderer::injectJavaScript(const QString &script) {
    webView->page()->runJavaScript(script, [](const QVariant &result) {
        // Optional callback for script execution result
    });
}

bool ArenaRenderer::createArenaHtmlFile(const QString &filePath) {
    QFile htmlFile(filePath);
    
    if (!htmlFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    // HTML with WebGL setup, fallback rendering and basic scene
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
            left: 5px;
            background-color: rgba(0,0,0,0.7);
            color: white;
            padding: 5px;
            font-family: monospace;
            border-radius: 3px;
            z-index: 100;
            font-size: 12px;
            max-width: 60%;
            white-space: pre-wrap;
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
        let player = {
            x: 0,
            y: 0.9,
            z: 0,
            rotation: 0
        };
        let arenaRadius = 10;
        let wallHeight = 2;
        let arenaRenderer;
        let webGLAvailable = true;
        let useFallback = false;
        let fallbackCanvas, fallbackCtx;
        let debugInfo = document.getElementById('debug-info');
        let lastUpdateTime = 0;

        // Enhanced WebGL detection with detailed logging
        function checkWebGL() {
            try {
                const canvas = document.createElement('canvas');
                const gl = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
                
                if (!gl) {
                    return false;
                }
                
                return true;
            } catch(e) {
                return false;
            }
        }
        
        // Update debug info display
        function updateDebugInfo(message) {
            if (debugInfo) {
                if (typeof message === 'object') {
                    try {
                        message = JSON.stringify(message, null, 2);
                    } catch (e) {
                        message = "Cannot display object: " + e.message;
                    }
                }
                debugInfo.textContent = message;
            }
        }
        
        // Fallback to basic canvas rendering when WebGL isn't available
        function initFallback() {
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
                
                if (!checkWebGL()) {
                    document.getElementById('error-container').style.display = 'block';
                    
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
                    document.getElementById('error-container').style.display = 'block';
                    
                    // Use fallback mode
                    initFallback();
                    
                    if (arenaRenderer) {
                        arenaRenderer.handleJavaScriptMessage("WebGL initialization failed, using fallback");
                    }
                }
            });
        });

        // Handle window resize events
        window.addEventListener('resize', onWindowResize);
    </script>
</body>
</html>
    )";
    
    // Write the HTML file
    htmlFile.write(htmlContent.toUtf8());
    htmlFile.close();
    
    return true;
}