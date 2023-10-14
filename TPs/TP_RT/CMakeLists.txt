if (CMAKE_HOST_WIN32)
    # need at least CMake 3.23 for IntelLLVM support of IntelDPCPP package on Windows
    cmake_minimum_required(VERSION 3.23)
else()
    # CMake 3.20.5 is the minimum recommended for IntelLLVM on Linux
    cmake_minimum_required(VERSION 3.20.5)
endif()

project(TP_RT LANGUAGES CXX)

find_package(IntelDPCPP REQUIRED)

file(GLOB SYCL_HEADERS "/opt/intel/oneapi/compiler/2023.2.1/linux/include/sycl/*.hpp" "/opt/intel/oneapi/compiler/2023.2.1/linux/include/sycl/CL/*.hpp")

add_executable(TP_RT
    ${SYCL_HEADERS}

    source/camera.cpp
    source/color.cpp
    source/image_io.cpp
    source/main.cpp
    source/mat.cpp
    source/triangle.cpp
    source/triangle.cpp
    source/vec.cpp

    include/camera.h
    include/color.h
    include/image.h
    include/image_io.h
    include/mat.h
    include/ray.h
    include/triangle.h
    include/vec.h

    stbi/stb_image.h
    stbi/stb_image_write.h)

#[===[
add_executable(
    TP_RT
    vector_test.cpp
    vector_test.h
    source/main.cpp)
]===]

target_include_directories(TP_RT PRIVATE "/opt/intel/oneapi/compiler/2023.2.1/linux/include/sycl/")
target_include_directories(TP_RT PRIVATE "include/")
target_include_directories(TP_RT PRIVATE "stbi/")
target_include_directories(TP_RT PRIVATE ".")
