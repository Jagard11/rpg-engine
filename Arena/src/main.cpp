#include <iostream>
#include <GL/glew.h>
#include "core/Game.hpp"
#include "core/StackTrace.hpp"

int main() {
    // Install signal handlers for better crash reporting
    Core::StackTrace::installSignalHandlers();
    
    try {
        Game game;
        
        if (!game.initialize()) {
            std::cerr << "Failed to initialize game" << std::endl;
            return -1;
        }

        // Start with splash screen, which will handle world creation/loading
        game.run();
        
        game.cleanup();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        Core::StackTrace::printStackTrace();
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        Core::StackTrace::printStackTrace();
        return -1;
    }
} 