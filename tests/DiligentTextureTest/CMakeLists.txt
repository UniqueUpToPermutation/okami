set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-diligent-texture-test ${SOURCE})

target_include_directories(okami-diligent-texture-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-diligent-texture-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)

add_test(NAME okami-diligent-texture-test COMMAND okami-diligent-texture-test)
add_dependencies(okami-tests okami-diligent-texture-test)

add_assets(okami-diligent-texture-test "assets")