// src/ui/arena_view.cpp
#include "../include/ui/arena_view.h"
#include "../include/rendering/gl_arena_widget.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QCoreApplication>
#include <QPainter>
#include <QDir>

// ArenaView constructor
ArenaView::ArenaView(CharacterManager *charManager, QWidget *parent)
    : QWidget(parent), characterManager(charManager), glWidget(nullptr) {
    
    // Set focus policy to receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
    
    try {
        // Create OpenGL widget instead of WebEngine renderer
        glWidget = new GLArenaWidget(characterManager, this);
        
        // Set up UI
        setupUI();
        
        // Connect signals - using the non-deprecated int version
        if (characterSelector) {
            connect(characterSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        if (index >= 0 && characterSelector) {
                            QString characterName = characterSelector->itemText(index);
                            onCharacterSelected(characterName);
                        }
                    });
        }
        
        if (resetButton) {
            connect(resetButton, &QPushButton::clicked, this, &ArenaView::onResetArena);
        }
        
        if (glWidget) {
            connect(glWidget, &GLArenaWidget::renderingInitialized, this, &ArenaView::onRendererInitialized);
        }
        
        // Use a timer to periodically re-grab focus if needed, but with a low frequency
        QTimer *focusTimer = new QTimer(this);
        connect(focusTimer, &QTimer::timeout, [this]() {
            // Only grab focus if this widget is visible and user is not interacting elsewhere
            if (isVisible() && !hasFocus() && glWidget && !glWidget->hasFocus() 
                && window() && window()->isActiveWindow()) {
                glWidget->setFocus();
            }
        });
        focusTimer->start(2000); // Check every two seconds - less aggressive
        
    } catch (const std::exception& e) {
        // Create a simplified UI with an error message
        QVBoxLayout *errorLayout = new QVBoxLayout(this);
        QLabel *errorLabel = new QLabel(
            "<h3>3D Visualization Unavailable</h3>"
            "<p>Your system does not have the required OpenGL capabilities.</p>"
            "<p>Please use the Conversation tab instead.</p>",
            this
        );
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLayout->addWidget(errorLabel);
        setLayout(errorLayout);
        
        // Set glWidget to nullptr to indicate it's not available
        glWidget = nullptr;
        
        // Rethrow the exception to notify the calling code
        throw;
    }
}

void ArenaView::initialize() {
    // Don't immediately initialize the arena - this will happen in onRendererInitialized
    // after the voxel system is properly set up
}

void ArenaView::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Top controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    QLabel *label = new QLabel("Character:", this);
    characterSelector = new QComboBox(this);
    resetButton = new QPushButton("Reset Arena", this);
    
    // Fix: Set focus policy for controls so they don't steal keyboard focus
    if (characterSelector) characterSelector->setFocusPolicy(Qt::ClickFocus);
    if (resetButton) resetButton->setFocusPolicy(Qt::ClickFocus);
    
    controlsLayout->addWidget(label);
    controlsLayout->addWidget(characterSelector);
    controlsLayout->addStretch();
    controlsLayout->addWidget(resetButton);
    
    // Controls label - Make more prominent
    controlsLabel = new QLabel(
        "<strong>Controls:</strong> W/S - Move forward/backward, A/D - Rotate left/right, Q/E - Strafe left/right, Mouse - Look",
        this
    );
    controlsLabel->setStyleSheet("background-color: rgba(0,0,0,0.1); padding: 5px; border-radius: 3px;");
    
    // Add OpenGL widget
    if (glWidget) {
        // Load characters into selector
        loadCharacters();
        
        // Add layouts and widgets
        mainLayout->addLayout(controlsLayout);
        mainLayout->addWidget(glWidget, 1); // Give view maximum stretch
        mainLayout->addWidget(controlsLabel);
    } else {
        // Create a simplified UI with no OpenGL widget
        QLabel *errorLabel = new QLabel("<h3>3D Visualization Unavailable</h3>", this);
        mainLayout->addWidget(errorLabel);
    }
    
    setLayout(mainLayout);
}

void ArenaView::loadCharacters() {
    if (!characterSelector) return;
    
    // Block signals during update to prevent multiple selections
    characterSelector->blockSignals(true);
    
    characterSelector->clear();
    
    // Add "None" option
    characterSelector->addItem("None", "");
    
    if (characterManager) {
        // Add all characters
        QStringList characters = characterManager->listCharacters();
        for (const QString &character : characters) {
            characterSelector->addItem(character, character);
        }
    }
    
    // Unblock signals when done
    characterSelector->blockSignals(false);
}

// Fix for the infinite recursion bug:
// Added a static flag to prevent recursive event forwarding
void ArenaView::keyPressEvent(QKeyEvent *event) {
    // Use a static flag to prevent infinite recursion
    static bool handlingKeyEvent = false;
    
    if (handlingKeyEvent) {
        // We're already handling a key event, just accept it and return
        event->accept();
        return;
    }
    
    // Set the flag before handling
    handlingKeyEvent = true;
    
    // Forward key events to OpenGL widget with safety checks
    if (glWidget && event) {
        // Don't call setFocus() inside the event handler - that can trigger another event
        
        // Call the key handler directly instead of using sendEvent
        glWidget->keyPressEvent(event);
        
        // Mark event as accepted to prevent it from being processed further
        event->accept();
    } else {
        // Fall back to default handling if no OpenGL widget
        QWidget::keyPressEvent(event);
    }
    
    // Reset the flag when done
    handlingKeyEvent = false;
}

// Fix the key release event handler in the same way
void ArenaView::keyReleaseEvent(QKeyEvent *event) {
    // Use a static flag to prevent infinite recursion
    static bool handlingKeyEvent = false;
    
    if (handlingKeyEvent) {
        // We're already handling a key event, just accept it and return
        event->accept();
        return;
    }
    
    // Set the flag before handling
    handlingKeyEvent = true;
    
    // Forward key events to OpenGL widget with safety checks
    if (glWidget && event) {
        // Call the key handler directly instead of using sendEvent
        glWidget->keyReleaseEvent(event);
        
        // Mark event as accepted to prevent it from being processed further
        event->accept();
    } else {
        // Fall back to default handling if no OpenGL widget
        QWidget::keyReleaseEvent(event);
    }
    
    // Reset the flag when done
    handlingKeyEvent = false;
}

void ArenaView::showEvent(QShowEvent *event) {
    // Make sure OpenGL widget has focus when shown, but delay slightly
    QTimer::singleShot(100, this, [this]() {
        if (glWidget && isVisible()) {
            glWidget->setFocus();
        }
    });
    
    QWidget::showEvent(event);
}

// Override focus events to debug focus issues
void ArenaView::focusInEvent(QFocusEvent *event) {
    // Forward focus to OpenGL widget if it exists
    if (glWidget) {
        glWidget->setFocus();
    }
    
    QWidget::focusInEvent(event);
}

void ArenaView::focusOutEvent(QFocusEvent *event) {
    // Let the base class handle the event
    QWidget::focusOutEvent(event);
}

void ArenaView::onCharacterSelected(const QString &characterName) {
    if (!glWidget) return;
    
    if (!characterName.isEmpty()) {
        loadCharacter(characterName);
    }
}

void ArenaView::onResetArena() {
    // Reset arena parameters and player position if OpenGL widget is available
    if (glWidget) {
        glWidget->initializeArena(20.0, 2.0);
        
        if (glWidget->getPlayerController()) {
            // Reset player by recreating the entity
            glWidget->getPlayerController()->createPlayerEntity();
        }
    }
    
    // Make sure OpenGL widget has focus after reset
    if (glWidget) {
        glWidget->setFocus();
    }
}

void ArenaView::onArenaParametersChanged() {
    // This would be connected to UI controls for arena parameters if added
}

void ArenaView::onRendererInitialized() {
    // Now that the renderer is initialized, it's safe to initialize the arena
    // The voxel system should be created by now
    if (glWidget) {
        // Use a short delay to ensure VoxelSystem is fully set up
        QTimer::singleShot(100, this, [this]() {
            glWidget->initializeArena(10.0, 2.0);
        });
    }
    
    // Delay loading the first character to ensure everything is initialized
    QTimer::singleShot(300, this, [this]() {
        // Load the first character if any are available
        if (characterSelector && characterSelector->count() > 1) {
            characterSelector->setCurrentIndex(1); // Select first actual character
        }
    });
    
    // Make sure OpenGL widget has focus
    QTimer::singleShot(500, this, [this]() {
        if (glWidget && isVisible()) {
            glWidget->setFocus();
        }
    });
}

void ArenaView::loadCharacter(const QString &characterName) {
    if (!glWidget || characterName.isEmpty()) return;
    
    // Set active character in OpenGL widget
    glWidget->setActiveCharacter(characterName);
    
    // Load character appearance with safety checks
    try {
        CharacterAppearance appearance;
        bool appearanceLoaded = false;
        
        if (characterManager) {
            try {
                appearance = characterManager->loadCharacterAppearance(characterName);
                appearanceLoaded = true;
            } catch (const std::exception& e) {
                qWarning() << "Error loading character appearance:" << e.what();
                // Continue with default appearance
            }
        }
        
        if (!appearanceLoaded) {
            // Set default values for appearance
            appearance.spritePath = "";
            appearance.collision.width = 1.0;
            appearance.collision.height = 2.0;
            appearance.collision.depth = 1.0;
        }
        
        // Use default sprite if none set
        if (appearance.spritePath.isEmpty()) {
            // Use absolute path for resources
            QString resourceDir = QDir::currentPath() + "/resources";
            QString defaultSpritePath = resourceDir + "/default_character.png";
            appearance.spritePath = defaultSpritePath;
            
            // Create default sprite file if it doesn't exist
            QFile defaultSprite(appearance.spritePath);
            if (!defaultSprite.exists()) {
                // Ensure resources directory exists
                QDir dir;
                if (!dir.exists(resourceDir)) {
                    if (!dir.mkpath(resourceDir)) {
                        qWarning() << "Failed to create resources directory:" << resourceDir;
                        
                        // Just use empty path to trigger missing texture visualization
                        if (glWidget) {
                            glWidget->loadCharacterSprite(characterName, "");
                        }
                        return;
                    }
                }
                
                try {
                    // Create a simple placeholder image
                    QImage image(128, 256, QImage::Format_ARGB32);
                    image.fill(Qt::transparent);
                    
                    // Draw a simple figure
                    QPainter painter(&image);
                    painter.setPen(Qt::black);
                    painter.setBrush(Qt::blue);
                    
                    // Head
                    painter.drawEllipse(QPoint(64, 40), 30, 30);
                    
                    // Body
                    painter.setBrush(Qt::red);
                    painter.drawRect(40, 70, 48, 100);
                    
                    // Arms
                    painter.setBrush(Qt::blue);
                    painter.drawRect(20, 70, 20, 80);
                    painter.drawRect(88, 70, 20, 80);
                    
                    // Legs
                    painter.drawRect(40, 170, 20, 80);
                    painter.drawRect(68, 170, 20, 80);
                    
                    if (!image.save(appearance.spritePath)) {
                        qWarning() << "Failed to save default sprite image";
                        // Use empty path to trigger missing texture visualization
                        if (glWidget) {
                            glWidget->loadCharacterSprite(characterName, "");
                        }
                        return;
                    }
                }
                catch (const std::exception& e) {
                    qWarning() << "Exception creating default sprite:" << e.what();
                    // Use empty path to trigger missing texture visualization
                    if (glWidget) {
                        glWidget->loadCharacterSprite(characterName, "");
                    }
                    return;
                }
            }
        }
        
        // Check if the sprite file exists
        QFile spriteFile(appearance.spritePath);
        if (!spriteFile.exists()) {
            qWarning() << "Sprite file does not exist at path:" << appearance.spritePath;
            // Use empty path to trigger missing texture visualization
            if (glWidget) {
                glWidget->loadCharacterSprite(characterName, "");
            }
            return;
        }
        
        // Load character sprite using OpenGL widget
        if (glWidget) {
            glWidget->loadCharacterSprite(characterName, appearance.spritePath);
        }
    } catch (const std::exception& e) {
        qWarning() << "Failed to load character sprite:" << e.what();
        // Still try to load with empty path for fallback
        if (glWidget) {
            glWidget->loadCharacterSprite(characterName, "");
        }
    } catch (...) {
        qWarning() << "Unknown exception in loadCharacter";
        // Still try to load with empty path for fallback
        if (glWidget) {
            glWidget->loadCharacterSprite(characterName, "");
        }
    }
}

// Return the player controller
PlayerController* ArenaView::getPlayerController() const {
    if (glWidget) {
        return glWidget->getPlayerController();
    }
    return nullptr;
}