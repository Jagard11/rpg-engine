// src/arena/core/rendering/arena_renderer_characters.cpp
#include "../../include/arena/core/rendering/arena_renderer.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>

void ArenaRenderer::loadCharacterSprite(const QString &characterName, const QString &texturePath) {
    if (!initialized) {
        qWarning() << "Cannot load sprite, renderer not initialized";
        return;
    }
    
    qDebug() << "Loading character sprite:" << characterName << "path:" << texturePath;
    
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
    createCharacterBillboard(characterName, texturePath, geometry);
}

void ArenaRenderer::createCharacterBillboard(const QString &characterName, 
                                          const QString &spritePath, 
                                          const CharacterCollisionGeometry &collisionGeometry) {
    QString js;
    
    // If spritePath is empty, we'll use the missing texture indicator
    if (spritePath.isEmpty()) {
        js = QString(
            "createCharacterBillboard('%1', '', %2, %3, %4);")
            .arg(characterName)
            .arg(collisionGeometry.width)
            .arg(collisionGeometry.height)
            .arg(collisionGeometry.depth);
    } else {
        // Check if sprite file exists
        QFile spriteFile(spritePath);
        if (!spriteFile.exists()) {
            qWarning() << "Sprite file does not exist:" << spritePath;
            js = QString(
                "createCharacterBillboard('%1', '', %2, %3, %4);")
                .arg(characterName)
                .arg(collisionGeometry.width)
                .arg(collisionGeometry.height)
                .arg(collisionGeometry.depth);
        } else {
            js = QString(
                "createCharacterBillboard('%1', '%2', %3, %4, %5);")
                .arg(characterName)
                .arg(spritePath)
                .arg(collisionGeometry.width)
                .arg(collisionGeometry.height)
                .arg(collisionGeometry.depth);
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

// Add JavaScript for character billboard creation to the scene
void ArenaRenderer::appendCharacterBillboardCode() {
    QString script = R"(
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
        
        // Debug output to console
        console.log(`Character ${characterName} positioned at: x=${x.toFixed(2)}, y=${y.toFixed(2)}, z=${z.toFixed(2)}`);
    }
    )";
    
    injectJavaScript(script);
}