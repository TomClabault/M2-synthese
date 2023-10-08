cmake_minimum_required(VERSION 3.10)

# set the project name
project(gkit2light)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

# gkit files
file(GLOB gkit_files "src/gKit/*.cpp")
# imgui files
file(GLOB imgui_files "src/imgui/*.cpp")
# project files files
file(GLOB TP_files "TPs/from_scratch/*.cpp")

# add the executable
add_executable(TP2 ${TP_files} ${gkit_files} ${imgui_files})
target_include_directories(TP2 PUBLIC "src/gKit")
target_include_directories(TP2 PUBLIC "src/imgui")

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(TP2 GL GLEW SDL2 SDL2_image OpenMP::OpenMP_CXX)
else()
    target_link_libraries(TP2 GL GLEW SDL2 SDL2_image)
endif()

set_target_properties(TP2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(TP2 PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
