@echo off
REM build.bat - Windows equivalent of build.sh
REM Configures and builds the OobaboogaRPGArena project for Windows

echo Current directory: %CD%
echo Script location: %~f0
echo --------------------------------------

echo Files in current directory:
dir /b

echo --------------------------------------

REM Check if build directory exists and remove it
if exist build (
    echo Removing existing build directory...
    rmdir /s /q build
)

REM Create build directory
echo Creating build directory...
mkdir build
cd build || (
    echo Failed to create/enter build directory
    exit /b 1
)

REM Set environment variables for OpenGL
set QT_OPENGL=desktop
set QT_LOGGING_RULES=qt.qpa.gl=true

REM Unset variables that might affect scaling
set QT_SCALE_FACTOR=
set QT_AUTO_SCREEN_SCALE_FACTOR=
set QT_SCREEN_SCALE_FACTORS=

REM Unset any variables that might force software rendering
set LIBGL_ALWAYS_INDIRECT=
set LIBGL_ALWAYS_SOFTWARE=
set LIBGL_DRI3_DISABLE=
set GALLIUM_DRIVER=

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo CMake not found. Please install CMake and add it to your PATH.
    echo Visit https://cmake.org/download/ to download CMake.
    exit /b 1
)

REM Check for Qt
where qmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo Qt not found. Please install Qt and add it to your PATH.
    echo Visit https://www.qt.io/download to download Qt.
    exit /b 1
)

REM Run CMake with proper options
echo Running CMake...
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo .. || (
    echo CMake failed. Trying Visual Studio generator...
    cmake -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebInfo .. || (
        echo CMake failed with both MinGW and Visual Studio generators.
        echo Please ensure you have a compatible compiler installed.
        exit /b 1
    )
)

REM Build the project
echo Building...
cmake --build . || (
    echo Build failed
    exit /b 1
)

REM Set explicit environment variables for OpenGL hardware acceleration
set QT_OPENGL=desktop
set QT_QUICK_BACKEND=software
set QSG_RENDER_LOOP=basic
set QTWEBENGINE_CHROMIUM_FLAGS=--disable-gpu
set QT_QPA_PLATFORM=windows

echo Build successful! Running application with hardware OpenGL enabled...
echo If you encounter OpenGL issues, try updating your graphics drivers.

REM Run the executable
.\OobaboogaRPGArena.exe