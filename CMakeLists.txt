cmake_minimum_required(VERSION 3.16)
project(EcosystemApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_library(colony_core
    src/core/content_loader.cpp
    src/core/localization_manager.cpp
    src/controllers/navigation_controller.cpp
)

target_include_directories(colony_core PUBLIC src third_party)

add_library(colony_ui
    src/frontend/components/empty_state_card.cpp
    src/frontend/components/sidebar_item.cpp
    src/frontend/components/search_field.cpp
    src/frontend/components/buttons.cpp
    src/frontend/components/badge.cpp
    src/frontend/components/brand_card.cpp
    src/frontend/components/theme_swatch.cpp
    src/frontend/utils/font_loader.cpp
    src/frontend/utils/icon_loader.cpp
    src/frontend/views/settings_view.cpp
    src/frontend/views/dashboard_page.cpp
    src/frontend/models/library_view_model.cpp
    src/ui/layout.cpp
    src/ui/theme.cpp
    src/ui/program_visuals.cpp
    src/ui/hub_panel.cpp
    src/ui/navigation.cpp
    src/ui/library_panel.cpp
    src/ui/hero_panel.cpp
    src/ui/settings_panel.cpp
    src/views/simple_text_view.cpp
    src/views/view_registry.cpp
    src/views/view_factory.cpp
    src/utils/drawing.cpp
    src/utils/color.cpp
    src/utils/font_manager.cpp
    src/utils/text_wrapping.cpp
)

target_include_directories(colony_ui PUBLIC src third_party)
target_link_libraries(colony_ui PUBLIC colony_core SDL2::SDL2 SDL2_ttf::SDL2_ttf)

add_library(colony_app
    src/app/application.cpp
)

target_include_directories(colony_app PUBLIC src third_party)
target_link_libraries(colony_app PUBLIC colony_ui)

add_executable(ecosystem_app
    src/main.cpp
)

target_link_libraries(ecosystem_app PRIVATE colony_app)

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

enable_testing()

add_executable(content_loader_tests tests/content_loader_tests.cpp)
target_include_directories(content_loader_tests PRIVATE src third_party)
target_link_libraries(content_loader_tests PRIVATE colony_app)
add_test(NAME content_loader_tests COMMAND content_loader_tests)
