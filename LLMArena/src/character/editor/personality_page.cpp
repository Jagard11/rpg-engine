// src/character/editor/personality_page.cpp
#include "../../../include/character/ui/character_editor.h"

#include <QFormLayout>

void CharacterEditorDialog::createPersonalityTab()
{
    QWidget *tab = new QWidget();
    QFormLayout *formLayout = new QFormLayout(tab);
    
    // Personality fields
    archetypeEdit = new QLineEdit(tab);
    traitsEdit = new QLineEdit(tab);
    valuesEdit = new QLineEdit(tab);
    fearsEdit = new QLineEdit(tab);
    desiresEdit = new QLineEdit(tab);
    quirksEdit = new QLineEdit(tab);
    speechPatternEdit = new QLineEdit(tab);
    backgroundEdit = new QTextEdit(tab);
    motivationEdit = new QTextEdit(tab);
    
    formLayout->addRow("Archetype:", archetypeEdit);
    formLayout->addRow("Traits (comma separated):", traitsEdit);
    formLayout->addRow("Values (comma separated):", valuesEdit);
    formLayout->addRow("Fears (comma separated):", fearsEdit);
    formLayout->addRow("Desires (comma separated):", desiresEdit);
    formLayout->addRow("Quirks:", quirksEdit);
    formLayout->addRow("Speech Pattern:", speechPatternEdit);
    formLayout->addRow("Background:", backgroundEdit);
    formLayout->addRow("Motivation:", motivationEdit);
    
    tab->setLayout(formLayout);
    tabWidget->addTab(tab, "Personality");
}

void CharacterEditorDialog::fillPersonalityFields(const CharacterPersonality &personality)
{
    archetypeEdit->setText(personality.archetype);
    traitsEdit->setText(personality.traits.join(", "));
    valuesEdit->setText(personality.values.join(", "));
    fearsEdit->setText(personality.fears.join(", "));
    desiresEdit->setText(personality.desires.join(", "));
    quirksEdit->setText(personality.quirks);
    speechPatternEdit->setText(personality.speechPattern);
    backgroundEdit->setText(personality.background);
    motivationEdit->setText(personality.motivation);
}

CharacterPersonality CharacterEditorDialog::collectPersonalityFields()
{
    CharacterPersonality personality;
    
    personality.archetype = archetypeEdit->text();
    personality.traits = traitsEdit->text().split(",", Qt::SkipEmptyParts);
    personality.values = valuesEdit->text().split(",", Qt::SkipEmptyParts);
    personality.fears = fearsEdit->text().split(",", Qt::SkipEmptyParts);
    personality.desires = desiresEdit->text().split(",", Qt::SkipEmptyParts);
    personality.quirks = quirksEdit->text();
    personality.speechPattern = speechPatternEdit->text();
    personality.background = backgroundEdit->toPlainText();
    personality.motivation = motivationEdit->toPlainText();
    
    return personality;
}