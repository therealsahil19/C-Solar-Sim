cmake_minimum_required(VERSION 3.14)
project(SolarSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-D_USE_MATH_DEFINES)

if(MSVC)
    add_compile_options(/O2 /arch:AVX2 /fp:fast)
else()
    add_compile_options(-O3 -mavx2 -ffast-math)
endif()

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

add_executable(benchmark
    src/benchmark.cpp
)

# GLM is header-only - include directories
target_include_directories(SolarSim PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm
)

target_include_directories(benchmark PRIVATE 
    include
)

# Link all libraries
target_link_libraries(SolarSim
    sfml-graphics
    sfml-window
    sfml-system
    ImGui-SFML::ImGui-SFML
    OpenGL::GL
)

# Custom command to copy DLLs to the build directory after build
add_custom_command(TARGET SolarSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE:ImGui-SFML::ImGui-SFML>
        $<TARGET_FILE_DIR:SolarSim>
    COMMENT "Copying runtime DLLs to executable directory..."
)


# Copy shaders to executable directory post-build
add_custom_command(TARGET SolarSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:SolarSim>/shaders
    COMMENT "Copying shaders..."
)

# Copy textures to executable directory post-build
add_custom_command(TARGET SolarSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/textures
        $<TARGET_FILE_DIR:SolarSim>/textures
    COMMENT "Copying textures..."
)

# Copy data to executable directory post-build
add_custom_command(TARGET SolarSim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/data
        $<TARGET_FILE_DIR:SolarSim>/data
    COMMENT "Copying data..."
)
