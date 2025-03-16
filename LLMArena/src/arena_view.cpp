// src/arena_view.cpp - Complete file with focus and key handling fixes
#include "../include/arena_view.h"
#include <QDebug>
#include <QMessageBox>
#include <QTimer>

// ArenaView constructor
ArenaView::ArenaView(CharacterManager *charManager, QWidget *parent)
    : QWidget(parent), characterManager(charManager) {
    
    // Set focus policy to receive keyboard events - MUST BE STRONGFOCUS
    setFocusPolicy(Qt::StrongFocus);
    
    try {
        // Create renderer - pass the CharacterManager to ArenaRenderer
        renderer = new ArenaRenderer(this, characterManager);
        
        // Set up UI
        setupUI();
        
        // Connect signals - using the non-deprecated int version
        connect(characterSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this](int index) {
                    QString characterName = characterSelector->itemText(index);
                    onCharacterSelected(characterName);
                });
        connect(resetButton, &QPushButton::clicked, this, &ArenaView::onResetArena);
        connect(renderer, &ArenaRenderer::renderingInitialized, this, &ArenaView::onRendererInitialized);
        
        // Use a timer to periodically re-grab focus if needed
        QTimer *focusTimer = new QTimer(this);
        connect(focusTimer, &QTimer::timeout, [this]() {
            // Only grab focus if this widget is visible and doesn't have it
            if (isVisible() && !hasFocus()) {
                qDebug() << "Regrabbing focus for ArenaView";
                setFocus();
                activateWindow();
            }
        });
        focusTimer->start(1000); // Check every second
        
    } catch (const std::exception& e) {
        qWarning() << "Failed to create ArenaRenderer:" << e.what();
        
        // Create a simplified UI with an error message
        QVBoxLayout *errorLayout = new QVBoxLayout(this);
        QLabel *errorLabel = new QLabel(
            "<h3>3D Visualization Unavailable</h3>"
            "<p>Your system does not have the required graphics capabilities.</p>"
            "<p>Please use the Conversation tab instead.</p>",
            this
        );
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLayout->addWidget(errorLabel);
        setLayout(errorLayout);
        
        // Set renderer to nullptr to indicate it's not available
        renderer = nullptr;
        
        // Rethrow the exception to notify the calling code
        throw;
    }
}

void ArenaView::initialize() {
    // Initialize the renderer if available
    if (renderer) {
        try {
            qDebug() << "Initializing arena renderer";
            renderer->initialize();
        } catch (const std::exception& e) {
            qWarning() << "Failed to initialize renderer:" << e.what();
            
            // Show error message
            QMessageBox::warning(this, "Renderer Initialization Failed", 
                                "Failed to initialize 3D renderer: " + QString(e.what()) + 
                                "\n\nPlease use the Conversation tab instead.");
        }
    }
}

void ArenaView::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Top controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    
    QLabel *label = new QLabel("Character:", this);
    characterSelector = new QComboBox(this);
    resetButton = new QPushButton("Reset Arena", this);
    
    // Fix: Set focus policy for controls so they don't steal keyboard focus
    characterSelector->setFocusPolicy(Qt::ClickFocus);
    resetButton->setFocusPolicy(Qt::ClickFocus);
    
    controlsLayout->addWidget(label);
    controlsLayout->addWidget(characterSelector);
    controlsLayout->addStretch();
    controlsLayout->addWidget(resetButton);
    
    // Controls label - Make more prominent
    controlsLabel = new QLabel(
        "<strong>Controls:</strong> W/S - Move forward/backward, A/D - Rotate left/right, Q/E - Strafe left/right",
        this
    );
    controlsLabel->setStyleSheet("background-color: rgba(0,0,0,0.1); padding: 5px; border-radius: 3px;");
    
    // Add WebView only if renderer is available
    if (renderer) {
        // Load characters into selector
        loadCharacters();
        
        // Add layouts and widgets
        mainLayout->addLayout(controlsLayout);
        mainLayout->addWidget(renderer->getView(), 1); // Give view maximum stretch
        mainLayout->addWidget(controlsLabel);
    } else {
        // Create a simplified UI with no WebView
        QLabel *errorLabel = new QLabel("<h3>3D Visualization Unavailable</h3>", this);
        mainLayout->addWidget(errorLabel);
    }
    
    setLayout(mainLayout);
}

void ArenaView::loadCharacters() {
    if (!characterSelector) return;
    
    characterSelector->clear();
    
    // Add "None" option
    characterSelector->addItem("None", "");
    
    // Add all characters
    QStringList characters = characterManager->listCharacters();
    for (const QString &character : characters) {
        characterSelector->addItem(character, character);
    }
}

void ArenaView::keyPressEvent(QKeyEvent *event) {
    qDebug() << "ArenaView received key press event: " << event->key();
    
    // Take focus when key is pressed
    setFocus();
    
    // Pass key press events to player controller if renderer is available
    if (renderer && renderer->getPlayerController()) {
        renderer->getPlayerController()->handleKeyPress(event);
        
        // Important: Accept the event to prevent it from being passed up
        event->accept();
    } else {
        // Call the base class implementation
        QWidget::keyPressEvent(event);
    }
}

void ArenaView::keyReleaseEvent(QKeyEvent *event) {
    qDebug() << "ArenaView received key release event: " << event->key();
    
    // Pass key release events to player controller if renderer is available
    if (renderer && renderer->getPlayerController()) {
        renderer->getPlayerController()->handleKeyRelease(event);
        
        // Important: Accept the event to prevent it from being passed up
        event->accept();
    } else {
        // Call the base class implementation
        QWidget::keyReleaseEvent(event);
    }
}

void ArenaView::showEvent(QShowEvent *event) {
    // Make sure we have focus when shown
    qDebug() << "ArenaView shown, setting focus";
    QTimer::singleShot(100, this, [this]() {
        setFocus();
        activateWindow();
    });
    
    QWidget::showEvent(event);
}

// Override focus events to debug focus issues
void ArenaView::focusInEvent(QFocusEvent *event) {
    qDebug() << "ArenaView received focus";
    QWidget::focusInEvent(event);
}

void ArenaView::focusOutEvent(QFocusEvent *event) {
    qDebug() << "ArenaView lost focus";
    QWidget::focusOutEvent(event);
}

void ArenaView::onCharacterSelected(const QString &characterName) {
    if (!renderer) return;
    
    if (!characterName.isEmpty()) {
        loadCharacter(characterName);
    }
}

void ArenaView::onResetArena() {
    // Reset arena parameters and player position if renderer is available
    if (renderer) {
        qDebug() << "Resetting arena to 10m radius with 2m walls";
        renderer->setArenaParameters(10.0, 2.0);
        
        if (renderer->getPlayerController()) {
            // Reset player by recreating the entity
            renderer->getPlayerController()->createPlayerEntity();
        }
    }
    
    // Make sure we have focus after reset
    setFocus();
}

void ArenaView::onArenaParametersChanged() {
    // This would be connected to UI controls for arena parameters if added
}

void ArenaView::onRendererInitialized() {
    qDebug() << "Renderer initialized - setting focus to arena view";
    
    // Load the first character if any are available
    if (characterSelector && characterSelector->count() > 1) {
        characterSelector->setCurrentIndex(1); // Select first actual character
    }
    
    // Make sure we have focus for key events
    QTimer::singleShot(500, this, [this]() {
        setFocus();
        activateWindow();
    });
}

void ArenaView::loadCharacter(const QString &characterName) {
    if (!renderer || characterName.isEmpty()) return;
    
    // Set active character in renderer
    renderer->setActiveCharacter(characterName);
    
    // Load character appearance
    CharacterAppearance appearance;
    try {
        appearance = characterManager->loadCharacterAppearance(characterName);
    } catch (const std::exception& e) {
        qWarning() << "Error loading character appearance:" << e.what();
        
        // Set default values for appearance
        appearance.spritePath = "";
        appearance.collision.width = 1.0;
        appearance.collision.height = 2.0;
        appearance.collision.depth = 1.0;
    }
    
    // Use default sprite if none set
    if (appearance.spritePath.isEmpty()) {
        qDebug() << "No sprite set for character, using default";
        
        // Use absolute path for resources
        QString resourceDir = QDir::currentPath() + "/resources";
        QString defaultSpritePath = resourceDir + "/default_character.png";
        appearance.spritePath = defaultSpritePath;
        
        qDebug() << "Default sprite path:" << defaultSpritePath;
        
        // Create default sprite file if it doesn't exist
        QFile defaultSprite(appearance.spritePath);
        if (!defaultSprite.exists()) {
            qDebug() << "Creating default sprite file";
            
            // Ensure resources directory exists
            QDir dir;
            if (!dir.exists(resourceDir)) {
                qDebug() << "Creating resources directory:" << resourceDir;
                if (!dir.mkpath(resourceDir)) {
                    qWarning() << "Failed to create resources directory:" << resourceDir;
                    
                    // Just use empty path to trigger missing texture visualization
                    renderer->loadCharacterSprite(characterName, "");
                    return;
                }
            }
            
            try {
                // Create a simple placeholder image
                QImage image(128, 256, QImage::Format_ARGB32);
                image.fill(Qt::transparent);
                
                qDebug() << "Drawing default sprite";
                
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
                
                qDebug() << "Saving default sprite to:" << appearance.spritePath;
                if (!image.save(appearance.spritePath)) {
                    qWarning() << "Failed to save default sprite image";
                    // Use empty path to trigger missing texture visualization
                    renderer->loadCharacterSprite(characterName, "");
                    return;
                }
                qDebug() << "Default sprite saved successfully";
            }
            catch (const std::exception& e) {
                qWarning() << "Exception creating default sprite:" << e.what();
                // Use empty path to trigger missing texture visualization
                renderer->loadCharacterSprite(characterName, "");
                return;
            }
        }
    }
    
    // Check if the sprite file exists
    QFile spriteFile(appearance.spritePath);
    if (!spriteFile.exists()) {
        qWarning() << "Sprite file does not exist at path:" << appearance.spritePath;
        // Use empty path to trigger missing texture visualization
        renderer->loadCharacterSprite(characterName, "");
        return;
    }
    
    try {
        // Load character sprite
        renderer->loadCharacterSprite(characterName, appearance.spritePath);
    } catch (const std::exception& e) {
        qWarning() << "Failed to load character sprite:" << e.what();
    }
}