set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND SOURCE
    main.cpp
)

add_executable(okami-hello-world ${SOURCE})

target_include_directories(okami-hello-world PUBLIC
    ${OKAMI_CORE_INCLUDE_DEPENDENCES})

target_link_libraries(okami-hello-world 
    ${OKAMI_CORE_LIB_DEPENDENCES})

add_test(NAME okami-hello-world COMMAND okami-hello-world)
add_dependencies(okami-tests okami-hello-world)