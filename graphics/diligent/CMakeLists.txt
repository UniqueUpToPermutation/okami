cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE shader_SRC CONFIGURE_DEPENDS 
    "shaders/*.hlsl" 
    "shaders/*.psh" 
    "shaders/*.gsh" 
    "shaders/*.fxh" 
    "shaders/*.vsh")

message("Embedded Shaders: ${shader_SRC}")

# Embed shaders (LINUX)
add_custom_command(
	OUTPUT shader_rc.cpp
	COMMAND embedfile ${CMAKE_CURRENT_SOURCE_DIR}/shaders shader_rc.cpp MakeShaderMap
	DEPENDS ${shader_SRC})

list(APPEND SOURCE
    src/BasicRenderer.cpp
    src/GraphicsUtils.cpp
    src/ShaderPreprocessor.cpp
    src/Shader.cpp
    src/Graphics.cpp
    src/SpriteBatch.cpp
    src/SpriteModule.cpp
    src/ImGuiSystem.cpp
    src/Im3dModule.cpp
    src/Im3dSystem.cpp
    src/TextureCapture.cpp
    src/FirstPersonCamera.cpp
    src/Im3dGizmo.cpp
    src/Editor.cpp
    src/Glfw.cpp
    src/RenderModule.cpp
    src/StaticMeshModule.cpp
    shader_rc.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IM3D_DIR}/im3d.cpp
)

if(PLATFORM_MACOS)
    list(APPEND SOURCE src/SurfaceHelper.mm)
endif()

list(APPEND INCLUDE
    include/okami/diligent/BasicRenderer.hpp
    include/okami/diligent/GraphicsUtils.hpp
    include/okami/diligent/ShaderPreprocessor.hpp
    include/okami/diligent/Shader.hpp
    include/okami/diligent/Buffers.hpp
    include/okami/diligent/SpriteBatch.hpp
    include/okami/diligent/SpriteModule.hpp
    include/okami/diligent/ImGuiSystem.hpp
    include/okami/diligent/RenderModule.hpp
    include/okami/diligent/Im3dModule.hpp
    include/okami/diligent/Im3dSystem.hpp
    include/okami/diligent/TextureCapture.hpp
    include/okami/diligent/FirstPersonCamera.hpp
    include/okami/diligent/Im3dGizmo.hpp
    include/okami/diligent/Glfw.hpp
)

add_library(okami-graphics-diligent STATIC ${SOURCE} ${INCLUDE})

set(OKAMI_DILIGENT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    CACHE INTERNAL OKAMI_DILIGENT_HEADERS)

list(APPEND DEPENDENCIES_LIST
    ${OKAMI_DILIGENT_HEADERS}
    ${OKAMI_GRAPHICS_INTERFACE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${DILIGENT_INCLUDE_PATHS}
)

SET(OKAMI_DILIGENT_INCLUDE_DEPENDENCES "${DEPENDENCIES_LIST}" CACHE INTERNAL "OKAMI_DILIGENT_INCLUDE_DEPENDENCES")

target_include_directories(okami-graphics-diligent PUBLIC
    ${CEREAL_INCLUDE_DIR} 
    ${ASSIMP_INCLUDE_DIR}
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_DILIGENT_INCLUDE_DEPENDENCES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IM3D_DIR}
)

target_compile_definitions(okami-graphics-diligent PRIVATE ${GLOBAL_COMPILER_FLAGS})

get_supported_backends(ENGINE_LIBRARIES)

target_link_libraries(okami-graphics-diligent
PRIVATE 
    Diligent-BuildSettings
PUBLIC
    Diligent-Common
    Diligent-GraphicsTools
    Diligent-TargetPlatform
    Diligent-Imgui
    ${OKAMI_CORE_LIB_DEPENDENCES}
    ${ENGINE_LIBRARIES}
    glfw
)

if(PLATFORM_UNIVERSAL_WINDOWS)
    target_link_libraries(okami-graphics-diligent PRIVATE dxguid.lib)
elseif(PLATFORM_ANDROID)
    target_link_libraries(okami-graphics-diligent PRIVATE GLESv3 PUBLIC native_app_glue)
elseif(PLATFORM_LINUX)
    target_link_libraries(okami-graphics-diligent PRIVATE XCBKeySyms GL X11)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set_source_files_properties(${IM3D_DIR}/im3d.cpp PROPERTIES COMPILE_FLAGS -Wno-everything)
endif()

list(APPEND LIB_DEPENDENCIES_LIST
    okami-graphics-diligent
)

SET(OKAMI_DILIGENT_LIB_DEPENDENCES "${LIB_DEPENDENCIES_LIST}" CACHE INTERNAL "OKAMI_DILIGENT_LIB_DEPENDENCES")
