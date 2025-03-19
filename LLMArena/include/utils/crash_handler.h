// include/utils/crash_handler.h
#ifndef CRASH_HANDLER_H
#define CRASH_HANDLER_H

#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QCoreApplication>
#include <QStandardPaths>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <execinfo.h>

// Max backtrace depth
constexpr int MAX_BACKTRACE_DEPTH = 50;

// Global crash handler functions
namespace CrashHandler {

// A global flag to check if crash handler is installed
static bool crashHandlerInstalled = false;

// C-style crash path for emergency use
static char g_crashLogPath[1024] = {0};

// Path for crash logs - use a guaranteed accessible location
static QString getCrashLogPath() {
    // Use the documents location for logs (or temp if that fails)
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (basePath.isEmpty()) {
        basePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }
    
    QString path = basePath + "/oobabooga_rpg_crash_logs";
    
    // Store the crash path in C-string for emergency use
    strncpy(g_crashLogPath, path.toStdString().c_str(), sizeof(g_crashLogPath) - 1);
    g_crashLogPath[sizeof(g_crashLogPath) - 1] = '\0';
    
    QDir dir(path);
    if (!dir.exists()) {
        bool created = dir.mkpath(path);
        std::cerr << "Creating crash log directory at: " << path.toStdString() 
                  << " - " << (created ? "Success" : "Failed") << std::endl;
    }
    
    return path;
}

// Low-level emergency file writing
static void writeEmergencyCrashLog(int signum) {
    // Get a timestamp string - use fixed buffer
    time_t now = time(nullptr);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", localtime(&now));
    
    // Construct path - use fixed buffer
    char filepath[1280];
    if (g_crashLogPath[0] != '\0') {
        snprintf(filepath, sizeof(filepath), "%s/crash_emergency_%s.log", g_crashLogPath, timestamp);
    } else {
        snprintf(filepath, sizeof(filepath), "/tmp/crash_emergency_%s.log", timestamp);
    }
    
    // Try to open a file
    FILE* f = fopen(filepath, "w");
    if (f) {
        // Write basic crash info
        fprintf(f, "=== EMERGENCY CRASH LOG ===\n");
        fprintf(f, "Time: %s\n", timestamp);
        fprintf(f, "Signal: %d\n", signum);
        
        // Get backtrace if possible
        void* bt[MAX_BACKTRACE_DEPTH];
        int bt_size = backtrace(bt, MAX_BACKTRACE_DEPTH);
        
        fprintf(f, "\n=== BACKTRACE ===\n");
        if (bt_size > 0) {
            char** bt_symbols = backtrace_symbols(bt, bt_size);
            if (bt_symbols) {
                for (int i = 0; i < bt_size; i++) {
                    fprintf(f, "[%d] %s\n", i, bt_symbols[i]);
                }
                free(bt_symbols);
            } else {
                fprintf(f, "Failed to get backtrace symbols\n");
            }
        } else {
            fprintf(f, "Failed to get backtrace\n");
        }
        
        fclose(f);
        fprintf(stderr, "Emergency crash log written to: %s\n", filepath);
    } else {
        fprintf(stderr, "CRITICAL: Failed to create emergency crash log at %s\n", filepath);
        
        // Try /tmp as absolute last resort
        snprintf(filepath, sizeof(filepath), "/tmp/crash_emergency_%s.log", timestamp);
        f = fopen(filepath, "w");
        if (f) {
            fprintf(f, "=== LAST RESORT EMERGENCY CRASH LOG ===\n");
            fprintf(f, "Time: %s\n", timestamp);
            fprintf(f, "Signal: %d\n", signum);
            fclose(f);
            fprintf(stderr, "Last resort crash log written to: %s\n", filepath);
        }
    }
}

// Signal handler for crashes - purely C-style for reliability
static void signalHandler(int signum) {
    // Print signal info to stderr immediately
    fprintf(stderr, "\n*** FATAL SIGNAL CAUGHT: %d ***\n", signum);
    
    // Write emergency crash log FIRST, using only C functions
    writeEmergencyCrashLog(signum);
    
    // Try Qt crash dump as secondary measure, but be safe
    static bool alreadyInHandler = false;
    if (!alreadyInHandler) {
        alreadyInHandler = true;
        
        try {
            // Do something with Qt if possible, but not critical
            if (QCoreApplication::instance()) {
                fprintf(stderr, "QApplication still available during crash\n");
            }
        } catch (...) {
            fprintf(stderr, "QApplication access failed during crash\n");
        }
    }
    
    // Reset signal handler and re-raise signal
    signal(signum, SIG_DFL);
    raise(signum);
}

// Write crash info to log file and console (legacy, may not be reliable)
static void dumpCrashInfo(const QString& reason, void** backtraceData = nullptr, int backtraceSize = 0) {
    std::cerr << "\n=== CRASH DETECTED: " << reason.toStdString() << " ===" << std::endl;
    
    // Low-level C style crash logging is more likely to work
    const char* reasonStr = reason.toStdString().c_str();
    char filepath[1280];
    
    if (g_crashLogPath[0] != '\0') {
        // Get timestamp using C functions
        time_t now = time(nullptr);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", localtime(&now));
        
        // Create path using C functions
        snprintf(filepath, sizeof(filepath), "%s/crash_%s.log", g_crashLogPath, timestamp);
        
        FILE* f = fopen(filepath, "w");
        if (f) {
            fprintf(f, "=== CRASH REPORT ===\n");
            fprintf(f, "Time: %s\n", timestamp);
            fprintf(f, "Reason: %s\n", reasonStr);
            
            if (backtraceData && backtraceSize > 0) {
                fprintf(f, "\n=== BACKTRACE ===\n");
                char** bt_symbols = backtrace_symbols(backtraceData, backtraceSize);
                if (bt_symbols) {
                    for (int i = 0; i < backtraceSize; i++) {
                        fprintf(f, "[%d] %s\n", i, bt_symbols[i]);
                    }
                    free(bt_symbols);
                }
            }
            
            fclose(f);
            fprintf(stderr, "Crash log written to: %s\n", filepath);
        } else {
            fprintf(stderr, "Failed to write crash log to: %s\n", filepath);
        }
    }
    
    // As a backup, also try Qt methods
    QString logPath = getCrashLogPath();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString qtFilePath = logPath + "/qt_crash_" + timestamp + ".log";
    
    QFile logFile(qtFilePath);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&logFile);
        stream << "=== QT CRASH REPORT ===\n";
        stream << "Time: " << QDateTime::currentDateTime().toString() << "\n";
        stream << "Reason: " << reason << "\n";
        
        if (backtraceData && backtraceSize > 0) {
            stream << "\n=== BACKTRACE ===\n";
            char** symbols = backtrace_symbols(backtraceData, backtraceSize);
            if (symbols) {
                for (int i = 0; i < backtraceSize; i++) {
                    stream << symbols[i] << "\n";
                }
                free(symbols);
            }
        }
        
        logFile.close();
    }
}

// Install signal handlers 
static void installHandlers() {
    try {
        std::cerr << "Installing crash handlers..." << std::endl;
        
        // Initialize the crash log path early
        QString pathStr = getCrashLogPath();
        
        // Direct C file access test
        {
            char testPath[1280];
            snprintf(testPath, sizeof(testPath), "%s/c_test_access.tmp", g_crashLogPath);
            
            FILE* testFile = fopen(testPath, "w");
            if (testFile) {
                fputs("C test", testFile);
                fclose(testFile);
                remove(testPath);
                std::cerr << "C-style file writing test successful" << std::endl;
            } else {
                std::cerr << "WARNING: C-style file writing test failed! Error: " << strerror(errno) << std::endl;
            }
        }
        
        // Qt file access test
        QFile test(pathStr + "/qt_test_access.tmp");
        if (test.open(QIODevice::WriteOnly | QIODevice::Text)) {
            test.write("Qt test");
            test.close();
            QFile::remove(pathStr + "/qt_test_access.tmp");
            std::cerr << "Qt file writing test successful" << std::endl;
        } else {
            std::cerr << "WARNING: Qt file writing test failed!" << std::endl;
        }
        
        // Set up Qt message handler if QApplication exists
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->setProperty("_qt_recent_logs", QString());
            qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
                QString formattedMsg = qFormatLogMessage(type, context, msg);
                std::cerr << formattedMsg.toStdString() << std::endl;
                
                if (QCoreApplication::instance()) {
                    QString recentLogs = QCoreApplication::instance()->property("_qt_recent_logs").toString();
                    QStringList logList = recentLogs.split("\n");
                    logList.append(formattedMsg);
                    while (logList.size() > 100) {
                        logList.removeFirst();
                    }
                    QCoreApplication::instance()->setProperty("_qt_recent_logs", logList.join("\n"));
                }
            });
        }
        
        // Install signal handlers for fatal signals
        std::signal(SIGSEGV, signalHandler);
        std::signal(SIGABRT, signalHandler);
        std::signal(SIGFPE, signalHandler);
        std::signal(SIGILL, signalHandler);
        std::signal(SIGBUS, signalHandler);
        
        // Write a marker file indicating successful installation
        {
            char markerPath[1280];
            snprintf(markerPath, sizeof(markerPath), "%s/handlers_installed.txt", g_crashLogPath);
            
            FILE* markerFile = fopen(markerPath, "w");
            if (markerFile) {
                time_t now = time(nullptr);
                char timestamp[64];
                strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
                
                fprintf(markerFile, "Crash handlers installed at %s\n", timestamp);
                fclose(markerFile);
            }
        }
        
        crashHandlerInstalled = true;
        std::cerr << "Crash handlers successfully installed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR installing crash handlers: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "UNKNOWN ERROR installing crash handlers" << std::endl;
    }
}

}  // namespace CrashHandler

#endif // CRASH_HANDLER_H