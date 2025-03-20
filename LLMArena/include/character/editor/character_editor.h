// include/character/editor/character_editor.h
#ifndef CHARACTER_EDITOR_H
#define CHARACTER_EDITOR_H

#include <QDialog>
#include <QMap>
#include <QVector>
#include <QTabWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTableWidget>
#include <QLabel>
#include <QListWidget>

#include "../manager/character_manager.h"

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
    
    // Memory management methods
    void addMemory();
    void editMemory();
    void deleteMemory();
    void testMemoryRetrieval();
    
    // Appearance methods
    void browseSprite();
    void updateSpritePreview(const QString &path);

private:
    // Tab creation methods
    void createBasicInfoTab();
    void createAppearanceTab();
    void createPersonalityTab();
    void createMemoriesTab();
    void create3DVisualizationTab();
    
    // Fill UI from character data
    void fillBasicInfoFields(const CharacterStats &stats);
    void fillAppearanceFields(const CharacterAppearance &appearance);
    void fillPersonalityFields(const CharacterPersonality &personality);
    void fill3DVisualizationFields(const CharacterAppearance &appearance);
    void fillMemoriesTable();
    
    // Collect character data from UI
    CharacterStats collectBasicInfoFields();
    CharacterAppearance collectAppearanceFields();
    CharacterPersonality collectPersonalityFields();
    CharacterAppearance collect3DVisualizationFields(CharacterAppearance appearance);

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
    
    // 3D Visualization tab
    QLineEdit *spritePathEdit;
    QDoubleSpinBox *widthSpin;
    QDoubleSpinBox *heightSpin;
    QDoubleSpinBox *depthSpin;
    QLabel *spritePreview;
    
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
    void newCharacter();
    void editCharacter();
    void deleteCharacter();
    void refreshCharacterList();
    
private:
    CharacterManager *characterManager;
    QListWidget *characterList;
};

#endif // CHARACTER_EDITOR_H