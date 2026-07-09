cmake_minimum_required(VERSION 3.20)

project(Ray-casting LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL3_ROOT "C:/Libraries/SDL3")

add_executable(Ray-casting
    src/main.cpp
)

target_include_directories(Ray-casting PRIVATE
    "${SDL3_ROOT}/include"
)

target_link_directories(Ray-casting PRIVATE
    "${SDL3_ROOT}/lib"
)

target_link_libraries(Ray-casting PRIVATE SDL3)

add_custom_command(TARGET Ray-casting POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL3_ROOT}/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:Ray-casting>"
)