cmake_minimum_required(VERSION 3.16)
project(Colony LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB COLONY_HEADERS
    CONFIGURE_DEPENDS
    include/colony/**/*.hpp
)

set(COLONY_SOURCES
    src/core/Manifest.cpp
    src/core/ModuleRegistry.cpp
    src/appcenter/AppCenter.cpp
    src/marketplace/MarketplaceClient.cpp
    src/security/SecurityManager.cpp
    src/installer/ModuleInstaller.cpp
    src/modules/TextEditorModule.cpp
    src/modules/AudioPlayerModule.cpp
    src/modules/ResourceManagerModule.cpp
)

add_library(colony STATIC ${COLONY_SOURCES} ${COLONY_HEADERS})

target_include_directories(colony
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(colony_demo src/main.cpp)

target_link_libraries(colony_demo PRIVATE colony)
