cmake_minimum_required(VERSION 3.6)
project(okami VERSION 0.1.0)

option(BUILD_CORE "Compile and build the core library" ON)
option(BUILD_GRAPHICS "Compile and build the graphics library" ON)
option(BUILD_TESTS "Compile and build the tests" ON)
option(BUILD_OKAMI_DILIGENT "Compile and build the DiligentEngine okami module" ON)
option(USE_GLFW "Use GLFW for windowing" ON)

add_compile_definitions(OKAMI)

add_subdirectory(ext)

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

if (BUILD_GRAPHICS)
    message("Configuring okami-graphics...")
    add_subdirectory(graphics)
endif()

function(add_assets TARGET_NAME ASSET_FOLDER)
    set_source_files_properties(${ASSET_FOLDER} PROPERTIES 
        VS_DEPLOYMENT_LOCATION "."
        MACOSX_PACKAGE_LOCATION "Resources" 
    )

    if(PLATFORM_WIN32 OR PLATFORM_LINUX OR PLATFORM_MACOS)
        # Copy assets to target folder
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/${ASSET_FOLDER}"
                "\"$<TARGET_FILE_DIR:${TARGET_NAME}>\"")
	endif()
endfunction()

if (BUILD_TESTS AND BUILD_CORE)
    message("Configuring okami-tests...")
    add_subdirectory(tests)
endif()

add_subdirectory(tools)