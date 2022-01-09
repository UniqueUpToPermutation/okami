set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-editor-test ${SOURCE})

target_include_directories(okami-editor-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-editor-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)

add_test(NAME okami-editor-test COMMAND okami-editor-test)
add_dependencies(okami-tests okami-editor-test)

add_assets(okami-editor-test "assets")