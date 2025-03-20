// src/character/editor/editor_core.cpp
#include "../../../include/character/ui/character_editor.h"
#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QGroupBox>

// Constructor
CharacterEditorDialog::CharacterEditorDialog(CharacterManager *manager, QWidget *parent)
    : QDialog(parent), characterManager(manager)
{
    setWindowTitle("Character Editor");
    setMinimumSize(800, 600);
    
    tabWidget = new QTabWidget(this);
    
    // Create tabs
    createBasicInfoTab();
    createAppearanceTab();
    createPersonalityTab();
    createMemoriesTab();
    create3DVisualizationTab();
    
    // Dialog buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &CharacterEditorDialog::saveCharacter);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    
    setLayout(mainLayout);
}

// Set the character to edit
void CharacterEditorDialog::setCharacter(const QString &name)
{
    characterName = name;
    
    if (!characterName.isEmpty()) {
        // Load existing character data
        CharacterStats stats = characterManager->loadCharacterStats(characterName);
        CharacterAppearance appearance = characterManager->loadCharacterAppearance(characterName);
        CharacterPersonality personality = characterManager->loadCharacterPersonality(characterName);
        memories = characterManager->loadMemories(characterName);
        
        // Fill UI fields with loaded data
        fillBasicInfoFields(stats);
        fillAppearanceFields(appearance);
        fillPersonalityFields(personality);
        fill3DVisualizationFields(appearance);
        fillMemoriesTable();
    }
}

// Save character data
void CharacterEditorDialog::saveCharacter()
{
    bool isNew = characterName.isEmpty();
    
    // Collect data from UI fields
    CharacterStats stats = collectBasicInfoFields();
    CharacterAppearance appearance = collectAppearanceFields();
    CharacterPersonality personality = collectPersonalityFields();
    
    // Update appearance with 3D visualization settings
    appearance = collect3DVisualizationFields(appearance);
    
    if (isNew) {
        // Create a new character
        characterName = stats.name;
        characterManager->createCharacter(characterName, stats, personality, appearance);
    } else {
        // Update existing character
        characterManager->saveCharacterStats(characterName, stats);
        characterManager->saveCharacterAppearance(characterName, appearance);
        characterManager->saveCharacterPersonality(characterName, personality);
    }
    
    // Save memories
    characterManager->saveMemories(characterName, memories);
    
    accept();
}

// CharacterManagerDialog implementation
CharacterManagerDialog::CharacterManagerDialog(CharacterManager *manager, QWidget *parent)
    : QDialog(parent), characterManager(manager)
{
    setWindowTitle("Character Manager");
    setMinimumSize(400, 300);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Character list
    characterList = new QListWidget(this);
    mainLayout->addWidget(characterList);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *newButton = new QPushButton("New Character", this);
    QPushButton *editButton = new QPushButton("Edit Character", this);
    QPushButton *deleteButton = new QPushButton("Delete Character", this);
    
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Close button
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(newButton, &QPushButton::clicked, this, &CharacterManagerDialog::newCharacter);
    connect(editButton, &QPushButton::clicked, this, &CharacterManagerDialog::editCharacter);
    connect(deleteButton, &QPushButton::clicked, this, &CharacterManagerDialog::deleteCharacter);
    connect(characterList, &QListWidget::itemDoubleClicked, this, &CharacterManagerDialog::editCharacter);
    
    // Initial population
    refreshCharacterList();
}

void CharacterManagerDialog::newCharacter()
{
    CharacterEditorDialog editor(characterManager, this);
    if (editor.exec() == QDialog::Accepted) {
        refreshCharacterList();
    }
}

void CharacterManagerDialog::editCharacter()
{
    QListWidgetItem *item = characterList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "No Selection", "Please select a character to edit.");
        return;
    }
    
    CharacterEditorDialog editor(characterManager, this);
    editor.setCharacter(item->text());
    
    if (editor.exec() == QDialog::Accepted) {
        refreshCharacterList();
    }
}

void CharacterManagerDialog::deleteCharacter()
{
    QListWidgetItem *item = characterList->currentItem();
    if (!item) {
        QMessageBox::warning(this, "No Selection", "Please select a character to delete.");
        return;
    }
    
    QString name = item->text();
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Deletion", 
        "Are you sure you want to delete character \"" + name + "\"?\nThis action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        if (characterManager->deleteCharacter(name)) {
            refreshCharacterList();
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete character.");
        }
    }
}

void CharacterManagerDialog::refreshCharacterList()
{
    characterList->clear();
    characterList->addItems(characterManager->listCharacters());
}