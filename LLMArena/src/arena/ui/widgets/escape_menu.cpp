#include "../../../../include/arena/ui/widgets/escape_menu.h"
#include <QDebug>

EscapeMenu::EscapeMenu(QWidget* parent)
    : QWidget(parent), m_performanceSettings(nullptr) {
    
    // Set object name for styling
    setObjectName("escapeMenu");
    
    // Setup UI
    setupUi();
    
    // Update UI state
    updateUI();
    
    // Hide by default
    hide();
    
    // Set window flags
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_StyledBackground, true); // Allow styling with stylesheets
    
    qDebug() << "EscapeMenu created with dimensions:" << width() << "x" << height();
}

void EscapeMenu::setupUi() {
    // Create main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(10);
    
    // Title
    QLabel* titleLabel = new QLabel("Game Menu");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 24px; color: white;");
    titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(titleLabel);
    
    // Tab widget for different settings
    m_tabWidget = new QTabWidget(this);
    m_mainLayout->addWidget(m_tabWidget);
    
    // Performance settings tab
    QWidget* performanceTab = new QWidget();
    QVBoxLayout* perfLayout = new QVBoxLayout(performanceTab);
    m_performanceSettings = new PerformanceSettingsWidget(this);
    m_performanceSettings->show();  // Make sure it's visible
    perfLayout->addWidget(m_performanceSettings);
    m_tabWidget->addTab(performanceTab, "Performance");
    
    // Controls buttons layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);
    
    // Resume button
    m_resumeButton = new QPushButton("Resume Game");
    m_resumeButton->setMinimumWidth(200);
    m_resumeButton->setMinimumHeight(50);
    connect(m_resumeButton, &QPushButton::clicked, this, &EscapeMenu::onResumeClicked);
    
    // Main menu button
    m_mainMenuButton = new QPushButton("Exit to Main Menu");
    m_mainMenuButton->setMinimumWidth(200);
    m_mainMenuButton->setMinimumHeight(50);
    connect(m_mainMenuButton, &QPushButton::clicked, this, &EscapeMenu::onMainMenuClicked);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_resumeButton);
    buttonLayout->addWidget(m_mainMenuButton);
    buttonLayout->addStretch();
    
    m_mainLayout->addLayout(buttonLayout);
    
    // Set background style - solid background for better visibility
    setStyleSheet("QWidget#escapeMenu { background-color: rgba(20, 20, 40, 230); }"
                 "QTabWidget::pane { border: 1px solid #666; background-color: rgba(30, 30, 50, 240); }"
                 "QTabBar::tab { background-color: #333; color: white; padding: 10px 20px; margin: 2px; font-size: 14px; }"
                 "QTabBar::tab:selected { background-color: #666; }"
                 "QPushButton { background-color: #444; color: white; padding: 10px; border: none; font-size: 16px; }"
                 "QPushButton:hover { background-color: #666; }"
                 "QLabel { color: white; }");
}

void EscapeMenu::toggleVisibility() {
    bool wasVisible = isVisible();
    
    qDebug() << "EscapeMenu toggleVisibility called, current visibility:" << wasVisible;
    
    if (wasVisible) {
        hide();
        qDebug() << "EscapeMenu hidden";
    } else {
        show();
        raise(); // Make sure it's on top
        qDebug() << "EscapeMenu shown with dimensions:" << width() << "x" << height();
        
        // Ensure performance settings widget is visible and updated
        if (m_performanceSettings) {
            m_performanceSettings->updateUiFromSettings();
            m_performanceSettings->show();
            qDebug() << "Performance settings updated and shown";
        } else {
            qDebug() << "Warning: Performance settings widget is null";
        }
    }
}

void EscapeMenu::onMainMenuClicked() {
    qDebug() << "Main menu button clicked";
    hide();
    emit returnToMainMenu();
}

void EscapeMenu::onResumeClicked() {
    qDebug() << "Resume button clicked";
    hide();
}

void EscapeMenu::updateUI() {
    qDebug() << "EscapeMenu updateUI called";
    
    // Update any UI elements based on current game state
    if (m_performanceSettings) {
        m_performanceSettings->updateUiFromSettings();
    }
} 