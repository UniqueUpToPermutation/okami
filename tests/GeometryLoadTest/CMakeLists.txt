set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-geometry-load-test ${SOURCE})

target_include_directories(okami-geometry-load-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES})

target_link_libraries(okami-geometry-load-test
    ${OKAMI_CORE_LIB_DEPENDENCES})

add_test(NAME okami-geometry-load-test COMMAND okami-geometry-load-test)
add_dependencies(okami-tests okami-geometry-load-test)

add_assets(okami-geometry-load-test "assets")