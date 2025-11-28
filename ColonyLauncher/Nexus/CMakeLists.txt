add_library(nexus STATIC
    src/nexus/arcade_main.cpp
)

target_include_directories(nexus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(nexus PUBLIC SDL2::SDL2)
