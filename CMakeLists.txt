cmake_minimum_required(VERSION 3.16)
project(EcosystemApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(ecosystem_app
    src/main.cpp
    src/content_loader.cpp
    src/controllers/navigation_controller.cpp
    src/utils/color.cpp
    src/utils/font_manager.cpp
    src/utils/text_wrapping.cpp
    src/ui/theme.cpp
    src/ui/program_visuals.cpp
    src/ui/navigation.cpp
    src/ui/library_panel.cpp
    src/ui/hero_panel.cpp
    src/ui/settings_panel.cpp
    src/views/simple_text_view.cpp
)

target_include_directories(ecosystem_app PRIVATE src third_party)

target_link_libraries(ecosystem_app PRIVATE SDL2::SDL2 SDL2_ttf::SDL2_ttf)

find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    target_compile_definitions(ecosystem_app PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(ecosystem_app PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(TARGET SDL2::SDL2main)
    target_link_libraries(ecosystem_app PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(ecosystem_app PRIVATE /W4 /permissive-)
else()
    target_compile_options(ecosystem_app PRIVATE -Wall -Wextra -Wpedantic)
endif()
