#ifndef ESCAPE_MENU_H
#define ESCAPE_MENU_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QTabWidget>

#include "performance_settings_widget.h"

/**
 * @brief A game escape menu that includes various settings and options
 * 
 * This menu appears when the user presses escape during gameplay and
 * provides access to game settings, performance options, and other controls.
 */
class EscapeMenu : public QWidget {
    Q_OBJECT
    
public:
    /**
     * @brief Construct an escape menu
     * @param parent Parent widget
     */
    explicit EscapeMenu(QWidget* parent = nullptr);
    
    /**
     * @brief Show the menu if hidden, hide if shown
     */
    void toggleVisibility();
    
signals:
    /**
     * @brief Signal emitted when the user requests to return to the main menu
     */
    void returnToMainMenu();
    
private slots:
    /**
     * @brief Handle main menu button click
     */
    void onMainMenuClicked();
    
    /**
     * @brief Handle resume button click
     */
    void onResumeClicked();
    
    /**
     * @brief Update UI state with current settings
     */
    void updateUI();
    
private:
    // Layout elements
    QVBoxLayout* m_mainLayout;
    QTabWidget* m_tabWidget;
    
    // Settings widget
    PerformanceSettingsWidget* m_performanceSettings;
    
    // Control buttons
    QPushButton* m_resumeButton;
    QPushButton* m_mainMenuButton;
    
    /**
     * @brief Setup the UI
     */
    void setupUi();
};

#endif // ESCAPE_MENU_H 