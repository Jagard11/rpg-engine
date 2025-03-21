// src/character/ui/character_manager_dialog.cpp
#include "../../../include/character/editor/character_editor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QMessageBox>
#include <QDialogButtonBox>

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