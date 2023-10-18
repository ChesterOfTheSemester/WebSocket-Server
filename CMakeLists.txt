cmake_minimum_required(VERSION 3.26)
project(CPPTEST)

set(CMAKE_CXX_STANDARD 17)

add_executable(CPPTEST src/main.cpp)

# After your add_executable line
target_link_libraries(CPPTEST ws2_32)
target_link_libraries(CPPTEST Crypt32.lib)

