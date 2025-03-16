// src/main.cpp
#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QDebug>

#include "../include/character_editor_ui.h"
#include "../include/character_persistence.h"
#include "../include/oobabooga_bridge.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create the main application window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Oobabooga RPG Arena");
    mainWindow.resize(800, 600);
    
    // Create central widget with layout
    QWidget* centralWidget = new QWidget(&mainWindow);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Initialize character manager
    CharacterManager* characterManager = new CharacterManager(&mainWindow);
    
    // Initialize Oobabooga bridge
    OobaboogaBridge* bridge = new OobaboogaBridge(characterManager, &mainWindow);
    
    // Load configuration
    bridge->loadConfig();
    
    // Create UI elements
    QPushButton* manageCharactersBtn = new QPushButton("Manage Characters", centralWidget);
    QPushButton* configureAPIBtn = new QPushButton("Configure API Connection", centralWidget);
    QPushButton* aboutBtn = new QPushButton("About", centralWidget);
    
    // Add buttons to layout
    mainLayout->addWidget(new QLabel("<h1>Oobabooga RPG Arena</h1>", centralWidget));
    mainLayout->addWidget(new QLabel("Connect RPG mechanics with LLM character interactions", centralWidget));
    mainLayout->addStretch();
    mainLayout->addWidget(manageCharactersBtn);
    mainLayout->addWidget(configureAPIBtn);
    mainLayout->addWidget(aboutBtn);
    mainLayout->addStretch();
    
    // Set up button connections
    QObject::connect(manageCharactersBtn, &QPushButton::clicked, [&]() {
        CharacterManagerDialog dialog(characterManager, &mainWindow);
        dialog.exec();
    });
    
    // Replace the configureAPIBtn click handler in main.cpp with this improved version
    QObject::connect(configureAPIBtn, &QPushButton::clicked, [&]() {
        // Simple dialog to configure API URL
        QString apiUrl = bridge->getApiUrl();
        bool ok;
        apiUrl = QInputDialog::getText(&mainWindow, "API Configuration",
                                    "Enter Oobabooga API URL:", 
                                    QLineEdit::Normal, 
                                    apiUrl.isEmpty() ? "http://localhost:5000" : apiUrl, &ok);
        if (ok && !apiUrl.isEmpty()) {
            // Ensure URL has a protocol
            if (!apiUrl.startsWith("http://") && !apiUrl.startsWith("https://")) {
                apiUrl = "http://" + apiUrl;
            }
            
            bridge->setApiUrl(apiUrl);
            bridge->saveConfig(apiUrl);
            
            // Test the connection
            bridge->testApiConnection();
        }
    });
    
    QObject::connect(aboutBtn, &QPushButton::clicked, [&]() {
        QMessageBox::about(&mainWindow, "About Oobabooga RPG Arena",
                          "Oobabooga RPG Arena\n\n"
                          "A tool for connecting LLMs with RPG mechanics.\n"
                          "Create persistent characters with memory and roleplay with them in a game environment.\n\n"
                          "Version: 0.1.0\n");
    });
    
    // Connect bridge signals
    QObject::connect(bridge, &OobaboogaBridge::statusMessage, [&](const QString &message) {
        QMessageBox::information(&mainWindow, "Status", message);
    });
    
    QObject::connect(bridge, &OobaboogaBridge::errorOccurred, [&](const QString &error) {
        QMessageBox::critical(&mainWindow, "Error", error);
    });
    
    // Set the central widget
    mainWindow.setCentralWidget(centralWidget);
    
    // Show the window
    mainWindow.show();
    
    return app.exec();
}