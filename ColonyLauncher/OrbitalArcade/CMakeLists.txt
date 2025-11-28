add_library(orbital_arcade STATIC
    src/orbital_arcade/arcade_main.cpp
)

target_include_directories(orbital_arcade PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(orbital_arcade PUBLIC SDL2::SDL2)
