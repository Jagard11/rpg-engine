# Voxel Game Engine

A modular voxel game engine inspired by Minecraft's creative mode, featuring procedural world generation and flying mechanics.

## Features

- Procedurally generated voxel world
- Creative mode movement with flying capabilities
- World saving and loading
- Customizable world generation seeds
- Modern OpenGL rendering

## Dependencies

- CMake 3.10 or higher
- C++17 compatible compiler
- GLFW3
- GLEW
- GLM
- OpenGL

## Building

### Linux

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install cmake libglfw3-dev libglew-dev libglm-dev

# Build
mkdir build
cd build
cmake ..
make
```

### Running

```bash
./VoxelGame
```

## Controls

- WASD - Movement
- Space - Jump/Fly up
- Left Shift - Fly down
- Double Space - Toggle flying
- Mouse - Look around
- Escape - Menu

## Project Structure

- `src/core/` - Core game systems
- `src/world/` - World generation and chunk management
- `src/player/` - Player movement and physics
- `src/renderer/` - Graphics rendering
- `src/ui/` - User interface elements
- `src/utils/` - Utility functions and helpers

## License

MIT License 