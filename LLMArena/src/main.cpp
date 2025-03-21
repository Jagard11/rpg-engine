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
#include <QProcess>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QTimer>

#include "../include/character/ui/character_editor_ui.h"
#include "../include/splash/ui/location_dialog.h"
#include "../include/character/core/character_persistence.h"
#include "../include/llm/oobabooga_bridge.h"
#include "../include/arena/ui/views/arena_view.h"
#include "../include/utils/crash_handler.h"

// Global crash handler to display errors and prevent immediate exit
void globalCrashHandler() {
    try {
        // Generate detailed crash log
        CrashHandler::dumpCrashInfo("Unhandled C++ exception");
        
        QMessageBox::critical(nullptr, "Application Error",
                             "An unhandled exception occurred. The application will now close.\n\n"
                             "A crash log has been saved to the crash_logs directory.\n"
                             "Please report this issue with the log file attached.");
    } catch (...) {
        // If even showing a message box fails, at least print to console
        qCritical() << "CRITICAL ERROR: Unhandled exception in application.";
    }
}

// Helper function to check if OpenGL is available and working
bool isOpenGLAvailable() {
    // Create a proper OpenGL context and check if it works
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3); // Require OpenGL 3.3 (core profile)
    format.setProfile(QSurfaceFormat::CoreProfile);
    
    QOpenGLContext context;
    context.setFormat(format);
    bool created = context.create();
    
    if (!created) {
        qWarning() << "Failed to create OpenGL context";
        return false;
    }
    
    // Check OpenGL version and vendor
    QOffscreenSurface surface;
    surface.setFormat(format);
    surface.create();
    
    if (!surface.isValid()) {
        qWarning() << "Failed to create valid surface";
        return false;
    }
    
    bool makeCurrent = context.makeCurrent(&surface);
    if (!makeCurrent) {
        qWarning() << "Failed to make OpenGL context current";
        return false;
    }
    
    // Get functions interface properly
    QOpenGLFunctions *f = context.functions();
    if (!f) {
        qWarning() << "Failed to get OpenGL functions";
        return false;
    }
    
    // Get OpenGL info safely
    const GLubyte* vendorStr = f->glGetString(GL_VENDOR);
    const GLubyte* rendererStr = f->glGetString(GL_RENDERER);
    const GLubyte* versionStr = f->glGetString(GL_VERSION);
    
    QString vendor = vendorStr ? QString::fromUtf8(reinterpret_cast<const char*>(vendorStr)) : "Unknown";
    QString renderer = rendererStr ? QString::fromUtf8(reinterpret_cast<const char*>(rendererStr)) : "Unknown";
    QString version = versionStr ? QString::fromUtf8(reinterpret_cast<const char*>(versionStr)) : "Unknown";
    
    qDebug() << "OpenGL Vendor:" << vendor;
    qDebug() << "OpenGL Renderer:" << renderer;
    qDebug() << "OpenGL Version:" << version;
    
    context.doneCurrent();
    
    // Simple check for software rendering (most software renderers include these terms)
    bool isSoftware = renderer.contains("llvmpipe", Qt::CaseInsensitive) || 
                     renderer.contains("software", Qt::CaseInsensitive) ||
                     renderer.contains("swrast", Qt::CaseInsensitive);
    
    // For this application, even software OpenGL is acceptable
    return true;
}

int main(int argc, char *argv[])
{
    // Set up global crash handler to catch unhandled exceptions
    std::set_terminate(globalCrashHandler);
    
    // Set environment variables to ensure OpenGL is properly configured
    qputenv("QT_OPENGL", "desktop");                  // Prefer desktop OpenGL
    qputenv("QSG_RENDER_LOOP", "basic");              // Use basic render loop for stability
    
    // Set OpenGL format for the entire application
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3); // Require OpenGL 3.3
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setSamples(0); // Disable multisampling for better compatibility
    QSurfaceFormat::setDefaultFormat(format);
    
    // High DPI support
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);  // Share OpenGL contexts
    
    // Initialize Qt application
    QApplication app(argc, argv);
    
    // IMPORTANT: Install signal handlers AFTER QApplication is initialized
    CrashHandler::installHandlers();
    
    // Check if OpenGL is available before trying to use it
    bool openglAvailable = isOpenGLAvailable();
    qDebug() << "OpenGL availability:" << openglAvailable;
    
    // Create the main application window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("RPG Arena (OpenGL)");
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
    QLabel* titleLabel = new QLabel("<h1>RPG Arena (OpenGL)</h1>", homeTab);
    QLabel* subtitleLabel = new QLabel("Connect RPG mechanics with LLM character interactions using native OpenGL rendering", homeTab);
    
    QPushButton* manageCharactersBtn = new QPushButton("Manage Characters", homeTab);
    QPushButton* configureAPIBtn = new QPushButton("Configure API Connection", homeTab);
    QPushButton* configureLocationBtn = new QPushButton("Set Location", homeTab);
    QPushButton* aboutBtn = new QPushButton("About", homeTab);
    
    // Add buttons to home layout
    homeLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    homeLayout->addWidget(subtitleLabel, 0, Qt::AlignCenter);
    homeLayout->addStretch();
    homeLayout->addWidget(manageCharactersBtn);
    homeLayout->addWidget(configureAPIBtn);
    homeLayout->addWidget(configureLocationBtn);
    homeLayout->addWidget(aboutBtn);
    homeLayout->addStretch();
    
    // Add home tab
    tabWidget->addTab(homeTab, "Home");
    
    // Create conversation tab for text-based interaction
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
    
    // Create a placeholder tab for the 3D arena that will be replaced later
    QWidget* arenaPlaceholder = new QWidget(tabWidget);
    QVBoxLayout* placeholderLayout = new QVBoxLayout(arenaPlaceholder);
    
    QLabel* loadingLabel = new QLabel("<h2>3D Arena Loading...</h2><p>Please wait while the OpenGL environment initializes.</p>", arenaPlaceholder);
    loadingLabel->setAlignment(Qt::AlignCenter);
    placeholderLayout->addWidget(loadingLabel);
    
    int arenaTabIndex = tabWidget->addTab(arenaPlaceholder, "3D Arena (Loading)");
    
    // Add tab widget to main layout
    mainLayout->addWidget(tabWidget);
    
    // Set up button connections
    QObject::connect(manageCharactersBtn, &QPushButton::clicked, [&]() {
        CharacterManagerDialog dialog(characterManager, &mainWindow);
        dialog.exec();
        
        // Update conversation character selector
        characterSelectorCombo->clear();
        characterSelectorCombo->addItem("None", "");
        characterSelectorCombo->addItems(characterManager->listCharacters());
    });
    
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
    
    // New connection for location configuration
    QObject::connect(configureLocationBtn, &QPushButton::clicked, [&]() {
        LocationDialog locationDialog(&mainWindow);
        if (locationDialog.exec() == QDialog::Accepted) {
            LocationData selectedLocation = locationDialog.getSelectedLocation();
            locationDialog.saveLocation(selectedLocation);
            
            // Show confirmation
            QMessageBox::information(&mainWindow, "Location Updated", 
                QString("Location set to %1\nLatitude: %2\nLongitude: %3")
                .arg(selectedLocation.name)
                .arg(selectedLocation.latitude)
                .arg(selectedLocation.longitude));
        }
    });
    
    QObject::connect(aboutBtn, &QPushButton::clicked, [&]() {
        QMessageBox::about(&mainWindow, "About RPG Arena (OpenGL)",
                          "RPG Arena (OpenGL)\n\n"
                          "A tool for connecting LLMs with RPG mechanics.\n"
                          "Create persistent characters with memory and roleplay with them in a game environment.\n\n"
                          "Features:\n"
                          "- Character persistence with memory system\n"
                          "- Native OpenGL 3D visualization\n"
                          "- Text-based conversation interface\n"
                          "- Integration with Oobabooga's LLM API\n\n"
                          "Version: 1.0.0");
    });
    
    // Connect bridge signals
    QObject::connect(bridge, &OobaboogaBridge::statusMessage, [&](const QString &message) {
        QMessageBox::information(&mainWindow, "Status", message);
    });
    
    QObject::connect(bridge, &OobaboogaBridge::errorOccurred, [&](const QString &error) {
        QMessageBox::critical(&mainWindow, "Error", error);
    });
    
    // Prevent user from clicking the 3D Arena tab until it's ready
    QObject::connect(tabWidget, &QTabWidget::currentChanged, [&mainWindow, tabWidget, arenaTabIndex](int index) {
        if (index == arenaTabIndex && tabWidget->tabText(index).contains("Loading")) {
            // Switch back to home tab
            tabWidget->setCurrentIndex(0);
            QMessageBox::information(&mainWindow, "Loading",
                                   "The 3D Arena is still loading. Please wait a moment.");
        }
    });
    
    // Set the central widget
    mainWindow.setCentralWidget(centralWidget);
    
    // Show the window before trying to create the arena view
    mainWindow.show();
    
    // Create ArenaView in a separate thread after a delay
    QTimer::singleShot(1000, [&mainWindow, &characterManager, tabWidget, arenaTabIndex]() {
        try {
            // Create an error tab for fallback
            QWidget* errorTab = new QWidget();
            QVBoxLayout* errorLayout = new QVBoxLayout(errorTab);
            
            try {
                // Create ArenaView and add to tabWidget
                ArenaView* arenaView = new ArenaView(characterManager, tabWidget);
                
                // Replace placeholder with actual arena view
                tabWidget->removeTab(arenaTabIndex);  
                tabWidget->insertTab(arenaTabIndex, arenaView, "3D Arena");
                
                // Delay initialization to give OpenGL context time to set up
                QTimer::singleShot(500, [arenaView]() {
                    try {
                        arenaView->initialize();
                    } catch (const std::exception& e) {
                        qCritical() << "Failed to initialize Arena View:" << e.what();
                    }
                });
            }
            catch (const std::exception& e) {
                qCritical() << "Failed to create 3D Arena view:" << e.what();
                
                // Add error message to the error tab
                QLabel* errorLabel = new QLabel(
                    "<h3>3D Visualization Unavailable</h3>"
                    "<p>Error: " + QString(e.what()) + "</p>"
                    "<p>Please use the Conversation tab instead.</p>"
                );
                errorLabel->setAlignment(Qt::AlignCenter);
                errorLayout->addWidget(errorLabel);
                
                // Replace placeholder with error tab
                tabWidget->removeTab(arenaTabIndex);
                tabWidget->insertTab(arenaTabIndex, errorTab, "3D Arena (Error)");
            }
            catch (...) {
                qCritical() << "Unknown exception creating 3D Arena view";
                
                // Add error message to the error tab
                QLabel* errorLabel = new QLabel(
                    "<h3>3D Visualization Unavailable</h3>"
                    "<p>An unknown error occurred.</p>"
                    "<p>Please use the Conversation tab instead.</p>"
                );
                errorLabel->setAlignment(Qt::AlignCenter);
                errorLayout->addWidget(errorLabel);
                
                // Replace placeholder with error tab
                tabWidget->removeTab(arenaTabIndex);
                tabWidget->insertTab(arenaTabIndex, errorTab, "3D Arena (Error)");
            }
        }
        catch (...) {
            qCritical() << "Critical error in ArenaView creation";
        }
    });
    
    // Run the application
    return app.exec();
}