cmake_minimum_required(VERSION 3.16)
project(ColonyLauncher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 COMPONENTS Widgets REQUIRED)

add_executable(colony_launcher
    src/main.cpp
    src/LauncherWindow.cpp
    src/CategoryAppsView.cpp
    src/ModulePlaceholder.cpp
)

target_include_directories(colony_launcher PRIVATE src)

qt_finalize_executable(colony_launcher)

target_link_libraries(colony_launcher PRIVATE Qt6::Widgets)
