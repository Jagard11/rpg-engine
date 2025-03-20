// src/character/editor/appearance_page.cpp
#include "../../../include/character/ui/character_editor.h"

#include <QFormLayout>
#include <QFileDialog>
#include <QPixmap>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>

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
    
    // Preview section
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

void CharacterEditorDialog::fill3DVisualizationFields(const CharacterAppearance &appearance)
{
    spritePathEdit->setText(appearance.spritePath);
    widthSpin->setValue(appearance.collision.width);
    heightSpin->setValue(appearance.collision.height);
    depthSpin->setValue(appearance.collision.depth);
    
    // Update preview
    updateSpritePreview(appearance.spritePath);
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