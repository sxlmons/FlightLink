cmake_minimum_required(VERSION 3.20)
project(FlightLink)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(Server)
add_subdirectory(Client)