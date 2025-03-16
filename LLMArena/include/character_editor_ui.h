// include/character_editor_ui.h
#ifndef CHARACTER_EDITOR_UI_H
#define CHARACTER_EDITOR_UI_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QGroupBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QSlider>
#include <QDateEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QDebug>

#include "character_persistence.h"

// Dialog to create or edit a character
class CharacterEditorDialog : public QDialog {
    Q_OBJECT

public:
    CharacterEditorDialog(CharacterManager *manager, QWidget *parent = nullptr);
    
    // Set the character to edit
    void setCharacter(const QString &name);
    
private slots:
    // Save character data
    void saveCharacter();
    
    // Add a new memory
    void addMemory();
    
    // Edit an existing memory
    void editMemory();
    
    // Delete a memory
    void deleteMemory();
    
    // Test memory retrieval
    void testMemoryRetrieval();

private:
    // Create the basic info tab
    void createBasicInfoTab();
    
    // Create the appearance tab
    void createAppearanceTab();
    
    // Create the personality tab
    void createPersonalityTab();
    
    // Create the memories tab
    void createMemoriesTab();
    
    // Fill in UI fields from character stats
    void fillBasicInfoFields(const CharacterStats &stats);
    
    // Fill in UI fields from character appearance
    void fillAppearanceFields(const CharacterAppearance &appearance);
    
    // Fill in UI fields from character personality
    void fillPersonalityFields(const CharacterPersonality &personality);
    
    // Fill in memories table from memories vector
    void fillMemoriesTable();
    
    // Collect basic info fields into character stats
    CharacterStats collectBasicInfoFields();
    
    // Collect appearance fields into character appearance
    CharacterAppearance collectAppearanceFields();
    
    // Collect personality fields into character personality
    CharacterPersonality collectPersonalityFields();

private:
    // UI elements
    QTabWidget *tabWidget;
    
    // Basic info tab
    QLineEdit *nameEdit;
    QLineEdit *raceEdit;
    QLineEdit *classEdit;
    QSpinBox *levelSpin;
    QMap<QString, QSpinBox*> attributeSpins;
    
    // Appearance tab
    QLineEdit *genderEdit;
    QLineEdit *ageEdit;
    QLineEdit *heightEdit;
    QLineEdit *buildEdit;
    QLineEdit *hairColorEdit;
    QLineEdit *hairStyleEdit;
    QLineEdit *eyeColorEdit;
    QLineEdit *skinToneEdit;
    QLineEdit *clothingEdit;
    QTextEdit *distinguishingFeaturesEdit;
    QTextEdit *generalDescriptionEdit;
    
    // Personality tab
    QLineEdit *archetypeEdit;
    QLineEdit *traitsEdit;
    QLineEdit *valuesEdit;
    QLineEdit *fearsEdit;
    QLineEdit *desiresEdit;
    QLineEdit *quirksEdit;
    QLineEdit *speechPatternEdit;
    QTextEdit *backgroundEdit;
    QTextEdit *motivationEdit;
    
    // Memories tab
    QTableWidget *memoriesTable;
    
    // Character data
    CharacterManager *characterManager;
    QString characterName;
    QVector<Memory> memories;
};

// Dialog to manage characters
class CharacterManagerDialog : public QDialog {
    Q_OBJECT

public:
    CharacterManagerDialog(CharacterManager *manager, QWidget *parent = nullptr);
    
private slots:
    // Create a new character
    void newCharacter();
    
    // Edit an existing character
    void editCharacter();
    
    // Delete a character
    void deleteCharacter();
    
    // Refresh the character list
    void refreshCharacterList();
    
private:
    CharacterManager *characterManager;
    QListWidget *characterList;
};

#endif // CHARACTER_EDITOR_UI_H