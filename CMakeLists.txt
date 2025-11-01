cmake_minimum_required(VERSION 3.16)
project(EcosystemApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(ecosystem_app
    src/main.cpp
)

target_link_libraries(ecosystem_app PRIVATE SDL2::SDL2)

if(TARGET SDL2::SDL2main)
    target_link_libraries(ecosystem_app PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(ecosystem_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(ecosystem_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
