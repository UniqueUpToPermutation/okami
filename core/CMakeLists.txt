cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    src/Frame.cpp
    src/Geometry.cpp
    src/ResourcePool.cpp
)

list(APPEND INCLUDE
    include/okami/Frame.hpp
    include/okami/Geometry.hpp
    include/okami/System.hpp
    include/okami/ResourcePool.hpp
)

add_library(okami-core STATIC ${SOURCE} ${INCLUDE})

target_include_directories(okami-core PUBLIC
    ${ENTT_INCLUDE_DIR} 
    ${MARL_INCLUDE_DIR} 
    ${CEREAL_INCLUDE_DIR} 
    ${ASSIMP_INCLUDE_DIR})

target_link_libraries(okami-core marl assimp)