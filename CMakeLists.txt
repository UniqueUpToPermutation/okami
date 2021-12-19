cmake_minimum_required(VERSION 3.6)
project(okami VERSION 0.1.0)

option(BUILD_CORE "Compile and build the core library" ON)
option(BUILD_GRAPHICS "Compile and build the graphics library" ON)
option(BUILD_TESTS "Compile and build the tests" ON)
option(BUILD_OKAMI_DILIGENT "Compile and build the DiligentEngine okami module" ON)
option(USE_GLFW "Use GLFW for windowing" ON)

add_subdirectory(ext)
add_subdirectory(tools)

include(CTest)

enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(PLATFORM_LINUX true CACHE INTERNAL "Is building on Linux")
    list(APPEND GLOBAL_COMPILER_FLAGS
        PLATFORM_LINUX)
    message("Linux detected as platform.")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" OR {CMAKE_SYSTEM_NAME} STREQUAL "Macos")
    SET(PLATFORM_MACOS true CACHE INTERNAL "Is building on Macos")
    list(APPEND GLOBAL_COMPILER_FLAGS
        PLATFORM_MACOS)
    message("MacOS detected as platform.")
endif()

if (USE_GLFW)
    list(APPEND GLOBAL_COMPILER_FLAGS
        USE_GLFW)
endif()

if (BUILD_CORE)
    message("Configuring okami-core...")
    add_subdirectory(core)
endif()

if (BUILD_TESTS AND BUILD_CORE)
    message("Configuring okami-tests...")
    add_subdirectory(tests)
endif()

if (BUILD_GRAPHICS)
    message("Configuring okami-graphics...")
    add_subdirectory(graphics)
endif()