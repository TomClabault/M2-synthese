if (CMAKE_HOST_WIN32)
    # need at least CMake 3.23 for IntelLLVM support of IntelDPCPP package on Windows
    cmake_minimum_required(VERSION 3.23)
else()
    # CMake 3.20.5 is the minimum recommended for IntelLLVM on Linux
    cmake_minimum_required(VERSION 3.20.5)
endif()

project(simple-dpcpp LANGUAGES CXX)

find_package(IntelDPCPP REQUIRED)

add_executable(simple-dpcpp simple.cpp)
