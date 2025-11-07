cmake_minimum_required(VERSION 3.16)
project(EcosystemApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(OpenSSL REQUIRED Crypto)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)
add_library(SODIUM::SODIUM INTERFACE IMPORTED)
set_target_properties(SODIUM::SODIUM PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${SODIUM_LIBRARIES}")

add_library(colony_core
    src/core/content_loader.cpp
    src/core/localization_manager.cpp
    src/controllers/navigation_controller.cpp
)

target_include_directories(colony_core PUBLIC src third_party)

add_library(colony_ui
    src/ui/theme.cpp
    src/ui/program_visuals.cpp
    src/ui/navigation.cpp
    src/ui/library_panel.cpp
    src/ui/hero_panel.cpp
    src/ui/settings_panel.cpp
    src/views/chart_view.cpp
    src/views/media_view.cpp
    src/views/simple_text_view.cpp
    src/views/view_registry.cpp
    src/views/view_factory.cpp
    src/utils/drawing.cpp
    src/utils/color.cpp
    src/utils/font_manager.cpp
    src/utils/text_wrapping.cpp
    src/utils/preferences.cpp
)

target_include_directories(colony_ui PUBLIC src third_party)
target_link_libraries(colony_ui PUBLIC colony_core SDL2::SDL2 SDL2_ttf::SDL2_ttf)

add_library(colony_app
    src/app/application.cpp
    src/programs/archive/ArchiveApp.cpp
)

target_include_directories(colony_app PUBLIC src third_party)
target_link_libraries(colony_app PUBLIC colony_ui OpenSSL::Crypto SODIUM::SODIUM)

add_executable(ecosystem_app
    src/main.cpp
)

target_link_libraries(ecosystem_app PRIVATE colony_app)

if(OpenSSL_FOUND)
    target_compile_definitions(ecosystem_app PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)
    target_link_libraries(ecosystem_app PRIVATE OpenSSL::SSL)
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

add_executable(archive_crypto_tests tests/archive/test_crypto.cpp)
target_include_directories(archive_crypto_tests PRIVATE src third_party)
target_link_libraries(archive_crypto_tests PRIVATE colony_app)
add_test(NAME archive_crypto_tests COMMAND archive_crypto_tests)

add_executable(archive_repository_tests tests/archive/test_repository.cpp)
target_include_directories(archive_repository_tests PRIVATE src third_party)
target_link_libraries(archive_repository_tests PRIVATE colony_app)
add_test(NAME archive_repository_tests COMMAND archive_repository_tests)
