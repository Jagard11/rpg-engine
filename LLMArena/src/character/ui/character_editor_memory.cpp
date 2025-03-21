// src/character/ui/character_editor_memory.cpp
#include "../../../include/character/editor/character_editor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>
#include <QDateTime>
#include <QRandomGenerator>

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