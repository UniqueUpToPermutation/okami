cmake_minimum_required(VERSION 3.6)

add_custom_target(okami-tests)

add_subdirectory(HelloWorld)
add_subdirectory(GeometryLoadTest)
add_subdirectory(UpdaterTest)

if (USE_GLFW)
    add_subdirectory(GLFWTest)
endif()

if (BUILD_OKAMI_DILIGENT)
    add_subdirectory(DiligentTest)
    add_subdirectory(DiligentGeometryTest)
    add_subdirectory(DiligentTextureTest)
    add_subdirectory(DiligentMeshPrefabTest)
    add_subdirectory(DiligentSpriteTest)
    add_subdirectory(DiligentImGuiTest)
    add_subdirectory(DiligentIm3dTest)
    add_subdirectory(DiligentFPSCameraTest)
    add_subdirectory(LightTest)
endif()