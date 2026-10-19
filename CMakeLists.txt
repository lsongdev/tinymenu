# One file for both consumers:
#   - ESP-IDF: put (or add as a git submodule) this repository under components/,
#     or depend on it through the component manager (idf_component.yml).
#   - Plain CMake: add_subdirectory(tinymenu) and link the `tinymenu` target.

set(TINYMENU_SRC_DIRS src/core src/gfx src/pages src/popups)

if(ESP_PLATFORM)
    # ESP-IDF also evaluates this file in script mode to collect requirements, where
    # file(GLOB ... CONFIGURE_DEPENDS) is not allowed; SRC_DIRS needs no glob.
    idf_component_register(SRC_DIRS ${TINYMENU_SRC_DIRS} INCLUDE_DIRS src)
    return()
endif()

cmake_minimum_required(VERSION 3.16)
project(tinymenu C)

file(GLOB_RECURSE TINYMENU_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
add_library(tinymenu STATIC ${TINYMENU_SOURCES})
target_include_directories(tinymenu PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src")
target_compile_features(tinymenu PUBLIC c_std_99)
