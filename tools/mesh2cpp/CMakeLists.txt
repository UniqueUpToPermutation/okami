set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(mesh2cpp CXX)

set(SOURCE
    mesh2cpp.cpp
)

add_executable(mesh2cpp ${SOURCE})

target_include_directories(mesh2cpp PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES})

target_link_libraries(mesh2cpp
    ${OKAMI_CORE_LIB_DEPENDENCES})

add_assets(mesh2cpp "assets")