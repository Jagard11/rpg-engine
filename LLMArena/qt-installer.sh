#!/bin/bash
# mint-qt-installer.sh - Script to install Qt5 dependencies for Linux Mint

echo "Installing Qt5 dependencies for Linux Mint..."

# Run the Ubuntu/Debian installation commands
sudo apt-get update
sudo apt-get install -y \
    qtbase5-dev \
    qtwebengine5-dev \
    libqt5webchannel5-dev \
    qttools5-dev \
    libqt5network5 \
    libqt5widgets5 \
    qt5-qmake

echo "Qt5 dependencies installed successfully! Press Enter to continue."
read