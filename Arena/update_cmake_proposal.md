# CMakeLists.txt Update Proposal

After removing the duplicate header files from the src/ directory, we should update the CMakeLists.txt file to enforce a cleaner architecture. This update will make it clearer that header files should only be placed in the include/ directory.

## Current Include Directories Configuration

```cmake
# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)
```

## Proposed Change

We should remove the `${CMAKE_CURRENT_SOURCE_DIR}/src` line to enforce that all headers should be in the include/ directory:

```cmake
# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)
```

## Implementation Steps

1. First run the `remove_duplicate_headers.sh` script to remove all duplicate header files
2. Make the above change to CMakeLists.txt
3. Rebuild the project to ensure everything still compiles correctly
4. Commit the changes

## Benefits

- Enforces a cleaner codebase structure
- Prevents accidental creation of headers in the src/ directory
- Makes it clear that include/ is the only place for header files
- Reduces the chance of include path confusion

## Risks and Mitigation

- **Risk**: Some files might still use includes with "src/" in their path
- **Mitigation**: Run a grep search for such includes and update them before making this change:

```bash
grep -r '#include "src/' . --include="*.cpp" --include="*.hpp"
```

- **Risk**: Hidden dependencies on the src/ include path
- **Mitigation**: If build fails after the change, we can temporarily revert the CMakeLists.txt file, fix the issues, and then reapply the change 