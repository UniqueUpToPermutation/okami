cmake_minimum_required(VERSION 3.6)

add_custom_target(okami-tests)

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

add_subdirectory(HelloWorld)
add_subdirectory(GeometryLoadTest)

if (USE_GLFW)
    add_subdirectory(GLFWTest)
endif()

if (BUILD_OKAMI_DILIGENT)
    add_subdirectory(DiligentTest)
    add_subdirectory(DiligentGeometryTest)
endif()