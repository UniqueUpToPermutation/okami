set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-transform-gizmo-test ${SOURCE})

target_include_directories(okami-transform-gizmo-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-transform-gizmo-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)

add_test(NAME okami-transform-gizmo-test COMMAND okami-transform-gizmo-test)
add_dependencies(okami-tests okami-transform-gizmo-test)

add_assets(okami-transform-gizmo-test "assets")