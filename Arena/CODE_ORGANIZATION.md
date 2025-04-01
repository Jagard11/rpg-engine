# Code Organization Guidelines

## Directory Structure

Our codebase follows these directory structure conventions:

```
rpg-engine/
├── include/        # Header files (.hpp)
│   ├── core/       # Core engine components
│   ├── world/      # World and chunk management
│   ├── renderer/   # Rendering system
│   └── ...
├── src/            # Implementation files (.cpp)
│   ├── core/       # Core engine implementations
│   ├── world/      # World and chunk implementations
│   ├── renderer/   # Rendering system implementations
│   └── ...
├── resources/      # Game resources (textures, models, etc.)
└── ...
```

## Key Guidelines

1. **Header Files Location**:
   - All header files (.hpp) should be placed in the appropriate subdirectory under `include/`
   - Never create header files in the `src/` directory

2. **Implementation Files Location**:
   - All implementation files (.cpp) should be placed in the appropriate subdirectory under `src/`
   - The directory structure should mirror the include directory structure

3. **Include Paths**:
   - When including headers from our codebase, use the directory relative to the include root:
     ```cpp
     #include "world/World.hpp"  // Correct
     #include "src/world/World.hpp"  // Incorrect
     ```

4. **Forward Declarations**:
   - Use forward declarations when possible to minimize header dependencies
   - Example:
     ```cpp
     // Instead of including the full header
     class Player;  // Forward declaration
     ```

5. **Interface Separation**:
   - Headers should contain class declarations, interface definitions, and inline functions
   - Implementation details should be kept in the corresponding .cpp file

## Modularity Best Practices

1. **Single Responsibility**:
   - Each class should have a single responsibility
   - Split large classes into smaller, more focused components

2. **Minimize Header Dependencies**:
   - Include only what's necessary in header files
   - Use the "Include What You Use" principle

3. **Hide Implementation Details**:
   - Use private implementation (pimpl) pattern for complex classes
   - Keep implementation details private where possible

4. **Consistent Naming**:
   - Use consistent naming conventions for files, classes, and methods
   - Match header filenames with their corresponding implementation files

## Build System

- Our CMakeLists.txt is configured to only look in the `include/` directory for headers
- This enforces the separation between interface and implementation

## Debugging

- When debugging complex issues, consider using GDB to generate backtraces:
  ```bash
  gdb --args ./VoxelGame
  ```
  Then when a crash occurs:
  ```
  (gdb) bt
  ```

By following these guidelines, we maintain a clean, modular, and maintainable codebase. 