set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-diligent-fps-camera-test ${SOURCE})

target_include_directories(okami-diligent-fps-camera-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-diligent-fps-camera-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)

add_test(NAME okami-diligent-fps-camera-test 
    COMMAND okami-diligent-fps-camera-test)
add_dependencies(okami-tests okami-diligent-fps-camera-test)

add_assets(okami-diligent-fps-camera-test "assets")