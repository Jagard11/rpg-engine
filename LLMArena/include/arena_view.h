// include/arena_view.h
#ifndef ARENA_VIEW_H
#define ARENA_VIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QKeyEvent>
#include "arena_renderer.h"
#include "character_persistence.h"
#include "player_controller.h"

// Main widget for the 3D arena view
class ArenaView : public QWidget {
    Q_OBJECT

public:
    ArenaView(CharacterManager *charManager, QWidget *parent = nullptr);
    
    // Initialize the arena
    void initialize();
    
    // Get the renderer
    ArenaRenderer* getRenderer() const { return renderer; }
    
    // Override key event handlers to pass to player controller
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    
protected:
    // Make sure widget gets focus
    void showEvent(QShowEvent *event) override;

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
    ArenaRenderer *renderer;
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