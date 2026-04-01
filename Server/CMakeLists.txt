cmake_minimum_required(VERSION 3.10)
project(FlightLinkServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server
    main.cpp
    source/connection_manager.cpp
    source/stream_processor.cpp
)

target_include_directories(server PRIVATE headers)
target_link_libraries(server ws2_32)
