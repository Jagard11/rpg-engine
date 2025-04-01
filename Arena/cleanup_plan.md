# Duplicate Header Cleanup Plan

## Issue
We've identified several duplicate header files between the `src/` and `include/` directories. This can cause confusion during development, potential compilation issues, and makes the codebase harder to maintain.

## Identified Duplicates
1. `include/world/World.hpp` and `src/world/World.hpp`
2. `include/world/Chunk.hpp` and `src/world/Chunk.hpp`
3. `include/world/ChunkVisibilityManager.hpp` and `src/world/ChunkVisibilityManager.hpp`
4. `include/renderer/Renderer.hpp` and `src/renderer/Renderer.hpp`

## Analysis
After examining these files:
- The src/ header files appear to contain only partial class declarations or implementations
- The include/ header files contain the complete declarations
- All source files appear to be including from the include/ directory

## Cleanup Steps
1. **Verify No Dependency on src/ Headers**:
   ```bash
   grep -r '#include ".*\.hpp"' src/ --include="*.cpp" | grep -v include/
   ```

2. **Backup Duplicates**:
   ```bash
   mkdir -p header_backup/world
   mkdir -p header_backup/renderer
   cp src/world/World.hpp header_backup/world/
   cp src/world/Chunk.hpp header_backup/world/
   cp src/world/ChunkVisibilityManager.hpp header_backup/world/
   cp src/renderer/Renderer.hpp header_backup/renderer/
   ```

3. **Remove Duplicate Headers**:
   ```bash
   git rm src/world/World.hpp
   git rm src/world/Chunk.hpp
   git rm src/world/ChunkVisibilityManager.hpp 
   git rm src/renderer/Renderer.hpp
   ```

4. **Update Include Paths**:
   If necessary, update any files that were including the src/ version of the headers.

5. **Rebuild and Test**:
   Ensure everything compiles and functions as expected.

## Benefits of This Cleanup
1. **Improved Modularity**: Clear separation between interface (include/) and implementation (src/)
2. **Reduced Confusion**: Developers know exactly where to find and modify header files
3. **Better Maintainability**: Changes to class interfaces only need to be made in one place
4. **Cleaner Codebase**: Standard organization leads to better readability

## Future Recommendations
1. Keep all header files with class declarations in the include/ directory
2. Only use .cpp files in the src/ directory (no .hpp files)
3. Consider using forward declarations to minimize header dependencies
4. Follow the "Include What You Use" principle to minimize unnecessary inclusions 