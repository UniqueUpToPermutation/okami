set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
)

add_executable(okami-diligent-pick-test ${SOURCE})

target_include_directories(okami-diligent-pick-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES}
    ${OKAMI_GRAPHICS_INTERFACE_DIR})

target_link_libraries(okami-diligent-pick-test
    ${OKAMI_CORE_LIB_DEPENDENCES}
    okami-graphics-diligent)
target_compile_definitions(okami-diligent-pick-test PUBLIC ${GLOBAL_COMPILER_FLAGS})

add_test(NAME okami-diligent-pick-test 
    COMMAND okami-diligent-pick-test)
add_dependencies(okami-tests okami-diligent-pick-test)

add_assets(okami-diligent-pick-test "assets")