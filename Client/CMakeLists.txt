cmake_minimum_required(VERSION 3.20)
project(Client)

set(CMAKE_CXX_STANDARD 23)

add_executable(Client main.cpp)

if(WIN32)
    target_link_libraries(Client ws2_32)
endif()
