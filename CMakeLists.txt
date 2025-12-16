cmake_minimum_required(VERSION 3.21)

project(warp-gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WaylandClient)
find_package(KF6WindowSystem REQUIRED)
find_package(LayerShellQt REQUIRED)

qt_standard_project_setup()

add_executable(warp-gui
    src/main.cpp
    src/popup_widget.cpp
    src/popup_widget.h
    src/settings_menu.cpp
    src/settings_menu.h
    src/toggle_switch.cpp
    src/toggle_switch.h
    src/tray_app.cpp
    src/tray_app.h
    src/warp_cli.cpp
    src/warp_cli.h
    src/wayland_popup_helper.cpp
    src/wayland_popup_helper.h
)

target_link_libraries(warp-gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WaylandClient
    KF6::WindowSystem
    LayerShellQt::Interface
)

# Enable Wayland platform integration
target_compile_definitions(warp-gui PRIVATE QT_WAYLAND_CLIENT_LIBRARY)
