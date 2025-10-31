cmake_minimum_required(VERSION 3.20)
project(AI_Movie_Studio VERSION 2.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FetchContent for external dependencies
include(FetchContent)

# Assimp for mesh loading (FBX, GLTF, OBJ, etc.)
message(STATUS "Fetching Assimp library...")
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.3
    GIT_SHALLOW TRUE
)

# Configure Assimp options
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)
message(STATUS "Assimp library configured successfully")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# DirectX 12 libraries (Windows SDK)
# These are available by default on Windows 10/11
# No need for find_package as DirectX 12 is part of Windows SDK

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create executable
add_executable(AI_Movie_Studio
    src/main.cpp
    src/core/Engine.cpp
    src/core/Logger.cpp
    src/core/Renderer.cpp
    src/core/ResourceManager.cpp
    src/core/InputSystem.cpp
    src/core/Camera.cpp
    src/core/Light.cpp
    src/core/Mesh.cpp
    src/core/Material.cpp
    src/core/AudioSystem.cpp
    src/core/SceneManager.cpp
    src/core/SceneObject.cpp
    src/core/SceneSerializer.cpp
    src/loaders/MeshLoader.cpp
    src/loaders/TextureLoader.cpp
    src/animation/AnimationCurve.cpp
    src/animation/AnimationClip.cpp
    src/animation/AnimationPlayer.cpp
    src/core/AnimationManager.cpp
    src/core/Command.cpp
    src/core/CommandHistory.cpp
    src/ai/AIClient.cpp
    src/ui/SceneHierarchy.cpp
    src/ui/AICommandProcessor.cpp
    src/ui/TimelineSystem.cpp
    src/ui/AssetBrowser.cpp
    src/ui/PropertiesPanel.cpp
    src/ui/MenuSystem.cpp
    src/ui/ToolbarSystem.cpp
    src/ui/UIManager.cpp
    src/ui/AIMovieStudioUI.cpp
    src/ui/UI.cpp
    src/ui/UIRenderer.cpp
    src/ui/ViewportRenderer.cpp
)

# Set properties
set_target_properties(AI_Movie_Studio PROPERTIES
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "AI_Movie_Studio"
)

# Link libraries
target_link_libraries(AI_Movie_Studio
    assimp
    d3d12
    dxgi
    d3dcompiler
    dxguid
    xaudio2
    winmm
    user32
    gdi32
    kernel32
    ole32
    oleaut32
    uuid
    comctl32
    comdlg32
    shell32
    version
    advapi32
    ws2_32
    mswsock
    crypt32
)

# Compiler definitions
target_compile_definitions(AI_Movie_Studio PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
    USE_ASSIMP  # Enable Assimp mesh loading
)

# Copy Assimp DLL to output directory after build
add_custom_command(TARGET AI_Movie_Studio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:assimp>
        $<TARGET_FILE_DIR:AI_Movie_Studio>
    COMMENT "Copying Assimp DLL to output directory"
)

# Compiler options
if(MSVC)
    target_compile_options(AI_Movie_Studio PRIVATE
        /W4
        /WX
        /permissive-
        /Zc:__cplusplus
        /Zc:externConstexpr
        /Zc:inline
        /Zc:preprocessor
        /Zc:referenceBinding
        /Zc:rvalueCast
        /Zc:strictStrings
        /Zc:ternary
        /Zc:throwingNew
        /Zc:twoPhase-
        /bigobj
        /MP
    )
    
    target_compile_options(AI_Movie_Studio PRIVATE
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
        $<$<CONFIG:Release>:/O2 /Oi /Ot /Oy /GL>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Ot /Oy /Zi>
        $<$<CONFIG:MinSizeRel>:/O1 /Oi /Ot /Oy /Ob2>
    )
    
    target_link_options(AI_Movie_Studio PRIVATE
        $<$<CONFIG:Release>:/LTCG>
    )
endif()

# Install target
install(TARGETS AI_Movie_Studio
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Copy DLLs to output directory
if(MSVC)
    add_custom_command(TARGET AI_Movie_Studio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:AI_Movie_Studio>"
        "${CMAKE_BINARY_DIR}/bin/"
    )
endif()