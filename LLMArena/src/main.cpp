// src/main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QLabel>
#include <QDebug>
#include <QSurfaceFormat>
#include <QWebEngineSettings>
#include <QtWebEngine/QtWebEngine>
#include <QProcess>
#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "../include/character_editor_ui.h"
#include "../include/character_persistence.h"
#include "../include/oobabooga_bridge.h"
#include "../include/arena_view.h"

// Helper function to check if OpenGL is available and working
bool isOpenGLAvailable() {
    // Create a minimal OpenGL context to check if it works
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 0); // OpenGL 2.0 is widely supported
    format.setProfile(QSurfaceFormat::NoProfile);
    
    QOffscreenSurface surface;
    surface.setFormat(format);
    surface.create();
    
    if (!surface.isValid()) {
        qWarning() << "Failed to create valid offscreen surface";
        return false;
    }
    
    QOpenGLContext context;
    context.setFormat(format);
    bool created = context.create();
    
    if (!created) {
        qWarning() << "Failed to create OpenGL context";
        return false;
    }
    
    bool makeCurrent = context.makeCurrent(&surface);
    if (!makeCurrent) {
        qWarning() << "Failed to make OpenGL context current";
        return false;
    }
    
    context.doneCurrent();
    return true;
}

int main(int argc, char *argv[])
{
    // Set QtWebEngine and OpenGL environment variables before anything else
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --disable-software-rasterizer");
    qputenv("QT_OPENGL", "software"); // Force software rendering
    
    // Enable high DPI scaling
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL); // Force software OpenGL
    
    // Important: Initialize QtWebEngine before creating QApplication
    QtWebEngine::initialize();
    
    // Set default surface format for OpenGL with software fallback
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(2, 0); // Use OpenGL 2.0 which is widely supported
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    QSurfaceFormat::setDefaultFormat(format);
    
    // Initialize Qt application
    QApplication app(argc, argv);
    
    // Check if OpenGL is available before trying to use it
    bool openglAvailable = isOpenGLAvailable();
    qDebug() << "OpenGL availability:" << openglAvailable;
    
    // Create the main application window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Oobabooga RPG Arena");
    mainWindow.resize(1024, 768);
    
    // Create central widget with layout
    QWidget* centralWidget = new QWidget(&mainWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Create tab widget for different views
    QTabWidget* tabWidget = new QTabWidget(centralWidget);
    
    // Initialize character manager
    CharacterManager* characterManager = new CharacterManager(&mainWindow);
    
    // Initialize Oobabooga bridge
    OobaboogaBridge* bridge = new OobaboogaBridge(characterManager, &mainWindow);
    
    // Load configuration
    bridge->loadConfig();
    
    // Create widgets for tabs
    QWidget* homeTab = new QWidget(tabWidget);
    QVBoxLayout* homeLayout = new QVBoxLayout(homeTab);
    
    // Create UI elements for home tab
    QLabel* titleLabel = new QLabel("<h1>Oobabooga RPG Arena</h1>", homeTab);
    QLabel* subtitleLabel = new QLabel("Connect RPG mechanics with LLM character interactions", homeTab);
    
    QPushButton* manageCharactersBtn = new QPushButton("Manage Characters", homeTab);
    QPushButton* configureAPIBtn = new QPushButton("Configure API Connection", homeTab);
    QPushButton* about3DBtn = new QPushButton("About 3D Arena", homeTab);
    QPushButton* aboutBtn = new QPushButton("About", homeTab);
    
    // Add buttons to home layout
    homeLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    homeLayout->addWidget(subtitleLabel, 0, Qt::AlignCenter);
    homeLayout->addStretch();
    homeLayout->addWidget(manageCharactersBtn);
    homeLayout->addWidget(configureAPIBtn);
    homeLayout->addWidget(about3DBtn);
    homeLayout->addWidget(aboutBtn);
    homeLayout->addStretch();
    
    // Add home tab
    tabWidget->addTab(homeTab, "Home");
    
    // Create conversation tab for text-based interaction when 3D is unavailable
    QWidget* conversationTab = new QWidget(tabWidget);
    QVBoxLayout* conversationLayout = new QVBoxLayout(conversationTab);
    
    QTextEdit* conversationHistoryText = new QTextEdit(conversationTab);
    conversationHistoryText->setReadOnly(true);
    
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QLineEdit* userInputField = new QLineEdit(conversationTab);
    QPushButton* sendButton = new QPushButton("Send", conversationTab);
    
    QComboBox* characterSelectorCombo = new QComboBox(conversationTab);
    characterSelectorCombo->addItem("None", "");
    characterSelectorCombo->addItems(characterManager->listCharacters());
    
    inputLayout->addWidget(userInputField);
    inputLayout->addWidget(sendButton);
    
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    QLabel* characterLabel = new QLabel("Character:", conversationTab);
    controlsLayout->addWidget(characterLabel);
    controlsLayout->addWidget(characterSelectorCombo);
    controlsLayout->addStretch();
    
    conversationLayout->addLayout(controlsLayout);
    conversationLayout->addWidget(conversationHistoryText);
    conversationLayout->addLayout(inputLayout);
    
    // Connect conversation UI signals/slots
    QObject::connect(sendButton, &QPushButton::clicked, [=]() {
        QString message = userInputField->text();
        if (message.isEmpty()) return;
        
        // Add user message to conversation
        conversationHistoryText->append("<b>You:</b> " + message);
        userInputField->clear();
        
        // Send to LLM
        bridge->sendMessageToLLM(message, "");
    });
    
    QObject::connect(userInputField, &QLineEdit::returnPressed, [=]() {
        sendButton->click();
    });
    
    QObject::connect(characterSelectorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
        [=](int index) {
            QString characterName = characterSelectorCombo->itemData(index).toString();
            bridge->setActiveCharacter(characterName);
            conversationHistoryText->append("<i>Character changed to: " + 
                (characterName.isEmpty() ? "None" : characterName) + "</i>");
        });
    
    QObject::connect(bridge, &OobaboogaBridge::responseReceived, [=](const QString &response) {
        QString character = bridge->getActiveCharacter();
        conversationHistoryText->append("<b>" + (character.isEmpty() ? "Assistant" : character) + 
                                     ":</b> " + response);
    });
    
    // Add the conversation tab
    tabWidget->addTab(conversationTab, "Conversation");
    
    // Try to create the 3D arena - with graceful fallback
    ArenaView* arenaView = nullptr;
    QWidget* fallbackWidget = nullptr;
    
    // Only try to create 3D arena if OpenGL is available
    if (openglAvailable) {
        try {
            // Create arena view but don't initialize immediately
            arenaView = new ArenaView(characterManager, &mainWindow);
            tabWidget->addTab(arenaView, "3D Arena");
        } catch (const std::exception& e) {
            qWarning() << "Failed to create 3D Arena view:" << e.what();
            openglAvailable = false;
        }
    }
    
    // If OpenGL is not available or arena creation failed, add fallback tab
    if (!openglAvailable || !arenaView) {
        fallbackWidget = new QWidget();
        QVBoxLayout* fallbackLayout = new QVBoxLayout(fallbackWidget);
        
        QLabel* errorLabel = new QLabel(
            "<h2>3D Arena Not Available</h2>"
            "<p>Your system does not have the required OpenGL support to run the 3D arena.</p>"
            "<p>Please use the Conversation tab instead for text-based character interactions.</p>"
            "<p>You can still create and manage characters as normal.</p>",
            fallbackWidget
        );
        errorLabel->setAlignment(Qt::AlignCenter);
        
        fallbackLayout->addWidget(errorLabel);
        tabWidget->addTab(fallbackWidget, "3D Arena (Unavailable)");
    }
    
    // Add tab widget to main layout
    mainLayout->addWidget(tabWidget);
    
    // Set up button connections
    QObject::connect(manageCharactersBtn, &QPushButton::clicked, [&]() {
        CharacterManagerDialog dialog(characterManager, &mainWindow);
        dialog.exec();
        
        // Refresh character lists
        if (arenaView) {
            arenaView->loadCharacters();
        }
        
        // Update conversation character selector
        characterSelectorCombo->clear();
        characterSelectorCombo->addItem("None", "");
        characterSelectorCombo->addItems(characterManager->listCharacters());
    });
    
    // Updated configureAPIBtn connection function
    QObject::connect(configureAPIBtn, &QPushButton::clicked, [&]() {
        // Simple dialog to configure API URL
        QString apiUrl = bridge->getApiUrl();
        bool ok;
        apiUrl = QInputDialog::getText(&mainWindow, "API Configuration",
                                    "Enter Oobabooga API URL (e.g., 0.0.0.0:5000):", 
                                    QLineEdit::Normal, 
                                    apiUrl.isEmpty() ? "0.0.0.0:5000" : apiUrl, &ok);
        if (ok && !apiUrl.isEmpty()) {
            // Remove any http:// prefix if the user included it
            if (apiUrl.startsWith("http://")) {
                apiUrl.remove(0, 7); // Remove "http://"
            } else if (apiUrl.startsWith("https://")) {
                apiUrl.remove(0, 8); // Remove "https://"
            }
            
            bridge->setApiUrl(apiUrl);
            bridge->saveConfig(apiUrl);
            
            // Test the connection
            bridge->testApiConnection();
        }
    });
    
    QObject::connect(about3DBtn, &QPushButton::clicked, [&]() {
        QMessageBox::about(&mainWindow, "About 3D Arena",
                         "3D Arena Mode\n\n"
                         "The 3D arena provides a Doom-style environment where characters are represented by billboarded sprites.\n\n"
                         "Controls:\n"
                         "W/S - Move forward/backward\n"
                         "A/D - Rotate left/right\n"
                         "Q/E - Strafe left/right\n\n"
                         "Character sprites can be configured in the Character Editor.\n\n"
                         "Note: 3D mode requires OpenGL 2.0 support. If your system doesn't support this,\n"
                         "you can still use the Conversation tab for text-based interactions.");
    });
    
    QObject::connect(aboutBtn, &QPushButton::clicked, [&]() {
        QMessageBox::about(&mainWindow, "About Oobabooga RPG Arena",
                          "Oobabooga RPG Arena\n\n"
                          "A tool for connecting LLMs with RPG mechanics.\n"
                          "Create persistent characters with memory and roleplay with them in a game environment.\n\n"
                          "Features:\n"
                          "- Character persistence with memory system\n"
                          "- 3D visualization with Doom-style billboarded sprites (when OpenGL is available)\n"
                          "- Text-based conversation interface\n"
                          "- Integration with Oobabooga's LLM API\n\n"
                          "Version: 0.2.1");
    });
    
    // Connect bridge signals
    QObject::connect(bridge, &OobaboogaBridge::statusMessage, [&](const QString &message) {
        QMessageBox::information(&mainWindow, "Status", message);
    });
    
    QObject::connect(bridge, &OobaboogaBridge::errorOccurred, [&](const QString &error) {
        QMessageBox::critical(&mainWindow, "Error", error);
    });
    
    // Connect tab changed signal to ensure focus for arena
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [&](int index) {
        if (arenaView && tabWidget->widget(index) == arenaView) {
            // Initialize if not already done
            arenaView->initialize();
            
            // Ensure focus for keyboard events
            arenaView->setFocus();
        }
    });
    
    // Set the central widget
    mainWindow.setCentralWidget(centralWidget);
    
    // Show the window
    mainWindow.show();
    
    return app.exec();
}