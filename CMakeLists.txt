cmake_minimum_required(VERSION 3.21)

# vcpkg toolchain must be set before project()
# VS2022 will auto-detect vcpkg if CMAKE_TOOLCHAIN_FILE is set in CMakePresets.json
# or if vcpkg is integrated system-wide

project(ImageConvertor VERSION 1.0.0 LANGUAGES CXX)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-specific settings
if(WIN32)
    # Unicode support
    add_definitions(-DUNICODE -D_UNICODE)
    
    # Win32 application (not console)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Find packages installed via vcpkg
# Note: vcpkg port name is libavif, CMake package name is also libavif
# But the CMake target to link is "avif"
find_package(libavif CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)

# Create executable
add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    # vcpkg libraries
    avif
    $<IF:$<TARGET_EXISTS:libjpeg-turbo::jpeg>,libjpeg-turbo::jpeg,libjpeg-turbo::jpeg-static>
    
    # Windows system libraries
    winhttp
    shell32
    ole32
)

# MSVC-specific compile options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4           # Warning level 4
        /permissive-  # Strict conformance mode
        /utf-8        # UTF-8 source and execution charset
    )
    
    # Use static runtime for Release builds (optional, remove if you prefer DLL runtime)
    # set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
)
