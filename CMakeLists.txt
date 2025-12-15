cmake_minimum_required(VERSION 3.23)

project(image_converter LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(image_converter WIN32
    src/main.cpp
)

target_compile_definitions(image_converter PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    target_compile_options(image_converter PRIVATE /W4 /permissive-)
endif()

find_package(libavif CONFIG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)

if(TARGET avif AND NOT TARGET libavif::avif)
    add_library(libavif::avif ALIAS avif)
endif()

if(TARGET turbojpeg AND NOT TARGET libjpeg-turbo::turbojpeg)
    add_library(libjpeg-turbo::turbojpeg ALIAS turbojpeg)
endif()

target_link_libraries(image_converter PRIVATE
    libavif::avif
    libjpeg-turbo::turbojpeg
    winhttp
    shell32
    ole32
)

target_include_directories(image_converter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
