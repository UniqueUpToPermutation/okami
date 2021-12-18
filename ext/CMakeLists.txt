cmake_minimum_required(VERSION 3.6)

if (BUILD_OKAMI_DILIGENT)
    add_subdirectory(DiligentEngine)

    if (USE_GLFW)
        SET(GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentSamples/ThirdParty/glfw/include"
            CACHE INTERNAL GLFW_INCLUDE_DIR)
    endif()

    LIST(APPEND DILIGENT_INCLUDES
        "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore/Common/interface"
        "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore/Graphics/GraphicsEngine/interface"
        "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore/Graphics/GraphicsEngineVulkan/interface"
        "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore/Graphics/GraphicsEngineOpenGL/interface"
        "${CMAKE_CURRENT_SOURCE_DIR}/DiligentEngine/DiligentCore/Platforms/Linux/interface"
    )

    SET(DILIGENT_INCLUDE_PATHS
        ${DILIGENT_INCLUDES}
        CACHE INTERNAL
        "DILIGENT_INCLUDE_PATHS"
    )
endif()

add_subdirectory(assimp)
add_subdirectory(marl)
add_subdirectory(entt)
add_subdirectory(glm)

SET(ASSIMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assimp/include" CACHE INTERNAL "ASSIMP_INCLUDE_DIR")
SET(MARL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/marl/include" CACHE INTERNAL "MARL_INCLUDE_DIR")
SET(ENTT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/entt/single_include" CACHE INTERNAL "ENTT_INCLUDE_DIR")
SET(CEREAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cereal/include" CACHE INTERNAL "CEREAL_INCLUDE_DIR")
SET(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glm" CACHE INTERNAL "GLM_INCLUDE_DIR")