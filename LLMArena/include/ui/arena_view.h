// include/ui/arena_view.h
#ifndef ARENA_VIEW_H
#define ARENA_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QKeyEvent>
#include <QFocusEvent>

#include "character_persistence.h"
#include "player_controller.h"

// Forward declarations
class GLArenaWidget;

// Main widget for the 3D arena view
class ArenaView : public QWidget {
    Q_OBJECT

public:
    ArenaView(CharacterManager *charManager, QWidget *parent = nullptr);
    
    // Initialize the arena
    void initialize();
    
    // Get the player controller
    PlayerController* getPlayerController() const;
    
    // Override key event handlers to pass to GL widget
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    
protected:
    // Make sure widget gets focus
    void showEvent(QShowEvent *event) override;
    
    // Track focus events
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

public slots:
    // Load characters into selector
    void loadCharacters();
    
private slots:
    // Handle character selection
    void onCharacterSelected(const QString &characterName);
    
    // Handle arena reset
    void onResetArena();
    
    // Handle arena parameters change
    void onArenaParametersChanged();
    
    // Handle renderer initialization
    void onRendererInitialized();

private:
    GLArenaWidget *glWidget;            // OpenGL widget for rendering
    CharacterManager *characterManager;
    QComboBox *characterSelector;
    QPushButton *resetButton;
    QLabel *controlsLabel;
    
    // Set up the UI
    void setupUI();
    
    // Load the selected character into the arena
    void loadCharacter(const QString &characterName);
};

#endif // ARENA_VIEW_H
