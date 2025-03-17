// src/ui/character_editor.cpp
#include "../include/ui/character_editor_ui.h"
#include "../include/character/character_persistence.h"
#include <QRandomGenerator> // Add missing include





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

void CharacterEditorDialog::createAppearanceTab()
{
    QWidget *tab = new QWidget();
    QFormLayout *formLayout = new QFormLayout(tab);
    
    // Physical appearance fields
    genderEdit = new QLineEdit(tab);
    ageEdit = new QLineEdit(tab);
    heightEdit = new QLineEdit(tab);
    buildEdit = new QLineEdit(tab);
    hairColorEdit = new QLineEdit(tab);
    hairStyleEdit = new QLineEdit(tab);
    eyeColorEdit = new QLineEdit(tab);
    skinToneEdit = new QLineEdit(tab);
    clothingEdit = new QLineEdit(tab);
    distinguishingFeaturesEdit = new QTextEdit(tab);
    generalDescriptionEdit = new QTextEdit(tab);
    
    formLayout->addRow("Gender:", genderEdit);
    formLayout->addRow("Age:", ageEdit);
    formLayout->addRow("Height:", heightEdit);
    formLayout->addRow("Build:", buildEdit);
    formLayout->addRow("Hair Color:", hairColorEdit);
    formLayout->addRow("Hair Style:", hairStyleEdit);
    formLayout->addRow("Eye Color:", eyeColorEdit);
    formLayout->addRow("Skin Tone:", skinToneEdit);
    formLayout->addRow("Clothing:", clothingEdit);
    formLayout->addRow("Distinguishing Features:", distinguishingFeaturesEdit);
    formLayout->addRow("General Description:", generalDescriptionEdit);
    
    tab->setLayout(formLayout);
    tabWidget->addTab(tab, "Appearance");
}

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

void CharacterEditorDialog::createMemoriesTab()
{
    QWidget *tab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(tab);
    
    // Memory table
    memoriesTable = new QTableWidget(tab);
    memoriesTable->setColumnCount(6);
    memoriesTable->setHorizontalHeaderLabels({"ID", "Date", "Title", "Type", "Intensity", "Last Recalled"});
    memoriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    memoriesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    memoriesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *addButton = new QPushButton("Add Memory", tab);
    QPushButton *editButton = new QPushButton("Edit Memory", tab);
    QPushButton *deleteButton = new QPushButton("Delete Memory", tab);
    QPushButton *testButton = new QPushButton("Test Retrieval", tab);
    
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addWidget(testButton);
    
    // Connect buttons
    connect(addButton, &QPushButton::clicked, this, &CharacterEditorDialog::addMemory);
    connect(editButton, &QPushButton::clicked, this, &CharacterEditorDialog::editMemory);
    connect(deleteButton, &QPushButton::clicked, this, &CharacterEditorDialog::deleteMemory);
    connect(testButton, &QPushButton::clicked, this, &CharacterEditorDialog::testMemoryRetrieval);
    
    mainLayout->addWidget(memoriesTable);
    mainLayout->addLayout(buttonLayout);
    
    tab->setLayout(mainLayout);
    tabWidget->addTab(tab, "Memories");
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

void CharacterEditorDialog::fillAppearanceFields(const CharacterAppearance &appearance)
{
    genderEdit->setText(appearance.gender);
    ageEdit->setText(appearance.age);
    heightEdit->setText(appearance.height);
    buildEdit->setText(appearance.build);
    hairColorEdit->setText(appearance.hairColor);
    hairStyleEdit->setText(appearance.hairStyle);
    eyeColorEdit->setText(appearance.eyeColor);
    skinToneEdit->setText(appearance.skinTone);
    clothingEdit->setText(appearance.clothing);
    distinguishingFeaturesEdit->setText(appearance.distinguishingFeatures);
    generalDescriptionEdit->setText(appearance.generalDescription);
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

void CharacterEditorDialog::fillMemoriesTable()
{
    memoriesTable->setRowCount(0);
    
    for (int i = 0; i < memories.size(); ++i) {
        const Memory &memory = memories[i];
        
        memoriesTable->insertRow(i);
        memoriesTable->setItem(i, 0, new QTableWidgetItem(memory.id));
        memoriesTable->setItem(i, 1, new QTableWidgetItem(memory.timestamp.toString("yyyy-MM-dd")));
        memoriesTable->setItem(i, 2, new QTableWidgetItem(memory.title));
        memoriesTable->setItem(i, 3, new QTableWidgetItem(memory.type));
        memoriesTable->setItem(i, 4, new QTableWidgetItem(QString::number(memory.emotionalIntensity)));
        memoriesTable->setItem(i, 5, new QTableWidgetItem(memory.lastRecalled.toString("yyyy-MM-dd")));
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

CharacterAppearance CharacterEditorDialog::collectAppearanceFields()
{
    CharacterAppearance appearance;
    
    appearance.gender = genderEdit->text();
    appearance.age = ageEdit->text();
    appearance.height = heightEdit->text();
    appearance.build = buildEdit->text();
    appearance.hairColor = hairColorEdit->text();
    appearance.hairStyle = hairStyleEdit->text();
    appearance.eyeColor = eyeColorEdit->text();
    appearance.skinTone = skinToneEdit->text();
    appearance.clothing = clothingEdit->text();
    appearance.distinguishingFeatures = distinguishingFeaturesEdit->toPlainText();
    appearance.generalDescription = generalDescriptionEdit->toPlainText();
    
    return appearance;
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

void CharacterEditorDialog::addMemory()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Add New Memory");
    dialog.setMinimumWidth(500);
    
    QFormLayout *formLayout = new QFormLayout(&dialog);
    
    QLineEdit *titleEdit = new QLineEdit(&dialog);
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"event", "conversation", "discovery", "reflection"});
    QTextEdit *descriptionEdit = new QTextEdit(&dialog);
    QLineEdit *emotionsEdit = new QLineEdit(&dialog);
    QSpinBox *intensitySpin = new QSpinBox(&dialog);
    intensitySpin->setRange(1, 10);
    intensitySpin->setValue(5);
    QLineEdit *locationsEdit = new QLineEdit(&dialog);
    QLineEdit *entitiesEdit = new QLineEdit(&dialog);
    QLineEdit *tagsEdit = new QLineEdit(&dialog);
    QLineEdit *relationshipsEdit = new QLineEdit(&dialog);
    
    formLayout->addRow("Title:", titleEdit);
    formLayout->addRow("Type:", typeCombo);
    formLayout->addRow("Description:", descriptionEdit);
    formLayout->addRow("Emotions (comma separated):", emotionsEdit);
    formLayout->addRow("Emotional Intensity (1-10):", intensitySpin);
    formLayout->addRow("Locations (comma separated):", locationsEdit);
    formLayout->addRow("Entities (comma separated):", entitiesEdit);
    formLayout->addRow("Tags (comma separated):", tagsEdit);
    formLayout->addRow("Relationships (comma separated):", relationshipsEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    formLayout->addRow(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        Memory memory;
        memory.id = QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + 
                  QString::number(QRandomGenerator::global()->bounded(1000));
        memory.timestamp = QDateTime::currentDateTime();
        memory.type = typeCombo->currentText();
        memory.title = titleEdit->text();
        memory.description = descriptionEdit->toPlainText();
        memory.emotions = emotionsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.emotionalIntensity = intensitySpin->value();
        memory.locations = locationsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.entities = entitiesEdit->text().split(",", Qt::SkipEmptyParts);
        memory.tags = tagsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.relationships = relationshipsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.lastRecalled = QDateTime::currentDateTime();
        memory.recallCount = 0;
        
        memories.append(memory);
        fillMemoriesTable();
    }
}

void CharacterEditorDialog::editMemory()
{
    int row = memoriesTable->currentRow();
    if (row < 0 || row >= memories.size()) {
        QMessageBox::warning(this, "No Selection", "Please select a memory to edit.");
        return;
    }
    
    Memory &memory = memories[row];
    
    QDialog dialog(this);
    dialog.setWindowTitle("Edit Memory");
    dialog.setMinimumWidth(500);
    
    QFormLayout *formLayout = new QFormLayout(&dialog);
    
    QLineEdit *titleEdit = new QLineEdit(memory.title, &dialog);
    QComboBox *typeCombo = new QComboBox(&dialog);
    typeCombo->addItems({"event", "conversation", "discovery", "reflection"});
    typeCombo->setCurrentText(memory.type);
    QTextEdit *descriptionEdit = new QTextEdit(&dialog);
    descriptionEdit->setText(memory.description);
    QLineEdit *emotionsEdit = new QLineEdit(memory.emotions.join(", "), &dialog);
    QSpinBox *intensitySpin = new QSpinBox(&dialog);
    intensitySpin->setRange(1, 10);
    intensitySpin->setValue(memory.emotionalIntensity);
    QLineEdit *locationsEdit = new QLineEdit(memory.locations.join(", "), &dialog);
    QLineEdit *entitiesEdit = new QLineEdit(memory.entities.join(", "), &dialog);
    QLineEdit *tagsEdit = new QLineEdit(memory.tags.join(", "), &dialog);
    QLineEdit *relationshipsEdit = new QLineEdit(memory.relationships.join(", "), &dialog);
    
    formLayout->addRow("Title:", titleEdit);
    formLayout->addRow("Type:", typeCombo);
    formLayout->addRow("Description:", descriptionEdit);
    formLayout->addRow("Emotions (comma separated):", emotionsEdit);
    formLayout->addRow("Emotional Intensity (1-10):", intensitySpin);
    formLayout->addRow("Locations (comma separated):", locationsEdit);
    formLayout->addRow("Entities (comma separated):", entitiesEdit);
    formLayout->addRow("Tags (comma separated):", tagsEdit);
    formLayout->addRow("Relationships (comma separated):", relationshipsEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    formLayout->addRow(buttonBox);
    
    if (dialog.exec() == QDialog::Accepted) {
        memory.type = typeCombo->currentText();
        memory.title = titleEdit->text();
        memory.description = descriptionEdit->toPlainText();
        memory.emotions = emotionsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.emotionalIntensity = intensitySpin->value();
        memory.locations = locationsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.entities = entitiesEdit->text().split(",", Qt::SkipEmptyParts);
        memory.tags = tagsEdit->text().split(",", Qt::SkipEmptyParts);
        memory.relationships = relationshipsEdit->text().split(",", Qt::SkipEmptyParts);
        
        fillMemoriesTable();
    }
}

void CharacterEditorDialog::deleteMemory()
{
    int row = memoriesTable->currentRow();
    if (row < 0 || row >= memories.size()) {
        QMessageBox::warning(this, "No Selection", "Please select a memory to delete.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Deletion", "Are you sure you want to delete this memory?",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        memories.remove(row);
        fillMemoriesTable();
    }
}

void CharacterEditorDialog::testMemoryRetrieval()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Test Memory Retrieval");
    dialog.setMinimumWidth(500);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    
    QFormLayout *formLayout = new QFormLayout();
    QTextEdit *contextEdit = new QTextEdit(&dialog);
    QLineEdit *entitiesEdit = new QLineEdit(&dialog);
    QLineEdit *locationsEdit = new QLineEdit(&dialog);
    QSpinBox *countSpin = new QSpinBox(&dialog);
    countSpin->setRange(1, 10);
    countSpin->setValue(5);
    
    formLayout->addRow("Current Context:", contextEdit);
    formLayout->addRow("Entities (comma separated):", entitiesEdit);
    formLayout->addRow("Locations (comma separated):", locationsEdit);
    formLayout->addRow("Number of Memories:", countSpin);
    
    QTextEdit *resultsEdit = new QTextEdit(&dialog);
    resultsEdit->setReadOnly(true);
    
    QPushButton *retrieveButton = new QPushButton("Retrieve Memories", &dialog);
    connect(retrieveButton, &QPushButton::clicked, [&]() {
        QStringList entities = entitiesEdit->text().split(",", Qt::SkipEmptyParts);
        QStringList locations = locationsEdit->text().split(",", Qt::SkipEmptyParts);
        
        QString results;
        
        if (!characterName.isEmpty()) {
            QString memoryContext = characterManager->generateMemoriesContext(
                characterName,
                contextEdit->toPlainText(),
                entities,
                locations,
                countSpin->value()
            );
            
            results = "RETRIEVED MEMORIES:\n\n" + memoryContext;
        } else {
            results = "No character selected.";
        }
        
        resultsEdit->setText(results);
    });
    
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(retrieveButton);
    mainLayout->addWidget(new QLabel("Results:"));
    mainLayout->addWidget(resultsEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    dialog.exec();
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
// Add these methods to src/character_editor.cpp

// Create a new tab for 3D visualization settings
void CharacterEditorDialog::create3DVisualizationTab()
{
    QWidget *tab = new QWidget();
    QFormLayout *formLayout = new QFormLayout(tab);
    
    // Sprite selection
    QHBoxLayout *spriteLayout = new QHBoxLayout();
    
    spritePathEdit = new QLineEdit(tab);
    QPushButton *browseSpriteBtn = new QPushButton("Browse...", tab);
    
    spriteLayout->addWidget(spritePathEdit);
    spriteLayout->addWidget(browseSpriteBtn);
    
    // Collision geometry settings
    QGroupBox *collisionGroup = new QGroupBox("Collision Geometry", tab);
    QFormLayout *collisionLayout = new QFormLayout(collisionGroup);
    
    widthSpin = new QDoubleSpinBox(tab);
    widthSpin->setRange(0.1, 10.0);
    widthSpin->setSingleStep(0.1);
    widthSpin->setValue(1.0);
    widthSpin->setSuffix(" m");
    
    heightSpin = new QDoubleSpinBox(tab);
    heightSpin->setRange(0.1, 10.0);
    heightSpin->setSingleStep(0.1);
    heightSpin->setValue(2.0);
    heightSpin->setSuffix(" m");
    
    depthSpin = new QDoubleSpinBox(tab);
    depthSpin->setRange(0.1, 10.0);
    depthSpin->setSingleStep(0.1);
    depthSpin->setValue(1.0);
    depthSpin->setSuffix(" m");
    
    collisionLayout->addRow("Width:", widthSpin);
    collisionLayout->addRow("Height:", heightSpin);
    collisionLayout->addRow("Depth:", depthSpin);
    
    collisionGroup->setLayout(collisionLayout);
    
    // Preview section (placeholder for future sprite preview)
    QLabel *previewLabel = new QLabel("Sprite Preview:", tab);
    spritePreview = new QLabel(tab);
    spritePreview->setMinimumSize(200, 200);
    spritePreview->setAlignment(Qt::AlignCenter);
    spritePreview->setFrameShape(QFrame::Box);
    spritePreview->setText("No sprite selected");
    
    // Add to form layout
    formLayout->addRow("Sprite Path:", spriteLayout);
    formLayout->addRow(collisionGroup);
    formLayout->addRow(previewLabel);
    formLayout->addRow(spritePreview);
    
    // Connect signals
    connect(browseSpriteBtn, &QPushButton::clicked, this, &CharacterEditorDialog::browseSprite);
    connect(spritePathEdit, &QLineEdit::textChanged, this, &CharacterEditorDialog::updateSpritePreview);
    
    tab->setLayout(formLayout);
    tabWidget->addTab(tab, "3D Visualization");
}

// Browse for a sprite image
void CharacterEditorDialog::browseSprite()
{
    QString filePath = QFileDialog::getOpenFileName(this, 
        "Select Character Sprite", 
        "", 
        "Image Files (*.png *.jpg *.jpeg *.bmp)");
    
    if (!filePath.isEmpty()) {
        spritePathEdit->setText(filePath);
    }
}

// Update sprite preview
void CharacterEditorDialog::updateSpritePreview(const QString &path)
{
    if (path.isEmpty()) {
        spritePreview->setText("No sprite selected");
        return;
    }
    
    QPixmap pixmap(path);
    if (pixmap.isNull()) {
        spritePreview->setText("Invalid image file");
        return;
    }
    
    // Scale pixmap to fit preview, maintaining aspect ratio
    pixmap = pixmap.scaled(spritePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    spritePreview->setPixmap(pixmap);
}

// Fill 3D visualization fields from character appearance
void CharacterEditorDialog::fill3DVisualizationFields(const CharacterAppearance &appearance)
{
    spritePathEdit->setText(appearance.spritePath);
    widthSpin->setValue(appearance.collision.width);
    heightSpin->setValue(appearance.collision.height);
    depthSpin->setValue(appearance.collision.depth);
    
    // Update preview
    updateSpritePreview(appearance.spritePath);
}

// Collect 3D visualization fields
CharacterAppearance CharacterEditorDialog::collect3DVisualizationFields(CharacterAppearance appearance)
{
    appearance.spritePath = spritePathEdit->text();
    
    CharacterCollisionGeometry geometry;
    geometry.width = widthSpin->value();
    geometry.height = heightSpin->value();
    geometry.depth = depthSpin->value();
    appearance.collision = geometry;
    
    return appearance;
}

// Update the CharacterEditorDialog::setCharacter method to include 3D visualization
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

// Update the CharacterEditorDialog::saveCharacter method to include 3D visualization
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

// Update the CharacterEditorDialog constructor to create the 3D visualization tab
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