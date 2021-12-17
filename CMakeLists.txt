cmake_minimum_required(VERSION 3.6)
project(okami VERSION 0.1.0)

include(CTest)

enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

option(BUILD_CORE "Compile and build the core library" ON)
option(BUILD_GRAPHICS "Compile and build the graphics library" ON)
option(BUILD_TESTS "Compile and build the tests" ON)
option(BUILD_OKAMI_DILIGENT "Compile and build the DiligentEngine okami module" ON)

add_subdirectory(ext)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    SET(PLATFORM_LINUX true)
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