set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-updater-test ${SOURCE})

target_include_directories(okami-updater-test PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES})

target_link_libraries(okami-updater-test 
    ${OKAMI_CORE_LIB_DEPENDENCES})

add_test(NAME okami-updater-test COMMAND okami-updater-test)
add_dependencies(okami-tests okami-updater-test)