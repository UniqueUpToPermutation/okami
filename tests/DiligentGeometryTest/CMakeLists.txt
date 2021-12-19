set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-diligent-geometry-test ${SOURCE})

target_include_directories(okami-diligent-geometry-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-diligent-geometry-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)

add_test(NAME okami-diligent-geometry-test COMMAND okami-diligent-geometry-test)
add_dependencies(okami-tests okami-diligent-geometry-test)