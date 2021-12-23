set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-diligent-sprite-test ${SOURCE})

target_include_directories(okami-diligent-sprite-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-diligent-sprite-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)

add_test(NAME okami-diligent-sprite-test 
    COMMAND okami-diligent-sprite-test)
add_dependencies(okami-tests okami-diligent-sprite-test)

add_assets(okami-diligent-sprite-test "assets")