cmake_minimum_required(VERSION 3.0.0)
project(embedfile VERSION 0.1.0)
add_executable(embedfile embedfile.cpp)
target_compile_features(embedfile PRIVATE cxx_std_17)