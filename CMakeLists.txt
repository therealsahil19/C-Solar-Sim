cmake_minimum_required(VERSION 3.14)
project(SolarSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_USE_MATH_DEFINES)

# =============================================================================
# Vendored Dependencies (no FetchContent needed - all committed to external/)
# =============================================================================

# Add SFML (graphics, windowing, system)
add_subdirectory(external/SFML)

# Add GLM (header-only math library for 3D graphics)
add_subdirectory(external/glm)

# Set IMGUI_DIR for ImGui-SFML to find ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)

# Add ImGui-SFML (requires SFML to be available)
set(IMGUI_SFML_FIND_SFML OFF)
add_subdirectory(external/imgui-sfml)

# =============================================================================
# Project Configuration
# =============================================================================

include_directories(include)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Add executable with custom GLAD loader
add_executable(SolarSim
    src/main.cpp
    src/glad.cpp
)

# GLM is header-only - include directories
target_include_directories(SolarSim PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
)

# Link all libraries
target_link_libraries(SolarSim
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
    OpenGL::GL
)

# Copy shaders to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy textures to build directory  
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/textures DESTINATION ${CMAKE_BINARY_DIR})

# Copy data to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
