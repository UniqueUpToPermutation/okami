cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    src/Frame.cpp
    src/Geometry.cpp
    src/Meta.cpp
    src/Transform.cpp
    src/Destroyer.cpp
    src/System.cpp
    src/Embed.cpp
    src/VertexFormat.cpp
    src/Texture.cpp
    src/Clock.cpp
    src/Graphics.cpp
    src/ResourceManager.cpp

    ../ext/lodepng/lodepng.cpp
)

list(APPEND INCLUDE
    include/okami/Frame.hpp
    include/okami/Geometry.hpp
    include/okami/System.hpp
    include/okami/ResourceBackend.hpp
    include/okami/Resource.hpp
    include/okami/Async.hpp
    include/okami/VertexFormat.hpp
    include/okami/Meta.hpp
    include/okami/Transform.hpp
    include/okami/Destroyer.hpp
    include/okami/ResourceManager.hpp
    include/okami/Okami.hpp
    include/okami/Embed.hpp
    include/okami/MipGenerator.hpp
    include/okami/Texture.hpp
    include/okami/Material.hpp
    include/okami/Clock.hpp
    include/okami/Event.hpp
    include/okami/PlatformDefs.hpp
    include/okami/BoundingBox.hpp
    include/okami/Observer.hpp
    include/okami/Graphics.hpp
)

add_library(okami-core STATIC ${SOURCE} ${INCLUDE})

SET(OKAMI_CORE_INCLUDE_DIR 
    "${CMAKE_CURRENT_SOURCE_DIR}/include" 
    CACHE INTERNAL "OKAMI_CORE_INCLUDE_DIR")

list(APPEND OKAMI_DEPENDENCIES_LIST 
    ${ENTT_INCLUDE_DIR} 
    ${MARL_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${OKAMI_CORE_INCLUDE_DIR}
)

SET(OKAMI_CORE_INCLUDE_DEPENDENCES 
    "${OKAMI_DEPENDENCIES_LIST}" 
    CACHE INTERNAL "OKAMI_CORE_INCLUDE_DEPENDENCES")

target_include_directories(okami-core PUBLIC
    ${CEREAL_INCLUDE_DIR} 
    ${ASSIMP_INCLUDE_DIR}
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LODEPNG_INCLUDE_DIR}
)

target_link_libraries(okami-core marl assimp)

list(APPEND OKAMI_LIB_DEPENDENCIES_LIST
    okami-core
    marl
)

SET(OKAMI_CORE_LIB_DEPENDENCES 
    "${OKAMI_LIB_DEPENDENCIES_LIST}" 
    CACHE INTERNAL "OKAMI_CORE_LIB_DEPENDENCES")
