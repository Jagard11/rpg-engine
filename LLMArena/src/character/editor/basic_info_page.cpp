// src/character/editor/basic_info_page.cpp
#include "../../../include/character/editor/character_editor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QMessageBox>

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

void CharacterEditorDialog::createBasicInfoTab()
{
    QWidget *tab = new QWidget();
    QFormLayout *formLayout = new QFormLayout(tab);
    
    // Basic character info
    nameEdit = new QLineEdit(tab);
    raceEdit = new QLineEdit(tab);
    classEdit = new QLineEdit(tab);
    levelSpin = new QSpinBox(tab);
    levelSpin->setRange(1, 100);
    
    formLayout->addRow("Name:", nameEdit);
    formLayout->addRow("Race:", raceEdit);
    formLayout->addRow("Class:", classEdit);
    formLayout->addRow("Level:", levelSpin);
    
    // Attributes section
    QGroupBox *attributesGroup = new QGroupBox("Attributes", tab);
    QGridLayout *attributesLayout = new QGridLayout(attributesGroup);
    
    // Standard D&D attributes
    QStringList attributes = {"Strength", "Dexterity", "Constitution", "Intelligence", "Wisdom", "Charisma"};
    
    int row = 0;
    for (const QString &attr : attributes) {
        QLabel *label = new QLabel(attr + ":", attributesGroup);
        QSpinBox *spin = new QSpinBox(attributesGroup);
        spin->setRange(1, 30);
        spin->setValue(10); // Default
        
        attributesLayout->addWidget(label, row, 0);
        attributesLayout->addWidget(spin, row, 1);
        
        attributeSpins[attr.toLower()] = spin;
        row++;
    }
    
    attributesGroup->setLayout(attributesLayout);
    formLayout->addRow(attributesGroup);
    
    tab->setLayout(formLayout);
    tabWidget->addTab(tab, "Basic Info");
}

void CharacterEditorDialog::fillBasicInfoFields(const CharacterStats &stats)
{
    nameEdit->setText(stats.name);
    raceEdit->setText(stats.race);
    classEdit->setText(stats.characterClass);
    levelSpin->setValue(stats.level);
    
    // Fill attributes
    for (auto it = stats.baseAttributes.constBegin(); it != stats.baseAttributes.constEnd(); ++it) {
        if (attributeSpins.contains(it.key())) {
            attributeSpins[it.key()]->setValue(it.value());
        }
    }
}

CharacterStats CharacterEditorDialog::collectBasicInfoFields()
{
    CharacterStats stats;
    
    stats.name = nameEdit->text();
    stats.race = raceEdit->text();
    stats.characterClass = classEdit->text();
    stats.level = levelSpin->value();
    
    // Collect attributes
    for (auto it = attributeSpins.constBegin(); it != attributeSpins.constEnd(); ++it) {
        stats.baseAttributes[it.key()] = it.value()->value();
    }
    
    return stats;
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