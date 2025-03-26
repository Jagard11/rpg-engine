#include <iostream>
#include <GL/glew.h>
#include "core/Game.hpp"

int main() {
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
        return -1;
    }
} 