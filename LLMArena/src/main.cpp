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
    
    // Updated configureAPIBtn connection function for main.cpp
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