cmake_minimum_required(VERSION 3.19)
project(hlmenu VERSION 1.0 DESCRIPTION "Application Menu using Hyprtoolkit")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(HYPRTOOLKIT REQUIRED IMPORTED_TARGET hyprtoolkit)
pkg_check_modules(HYPRUTILS REQUIRED IMPORTED_TARGET hyprutils)
pkg_check_modules(AQUAMARINE REQUIRED IMPORTED_TARGET aquamarine)
pkg_check_modules(HYPRGRAPHICS REQUIRED IMPORTED_TARGET hyprgraphics)
pkg_check_modules(PIXMAN REQUIRED IMPORTED_TARGET pixman-1)
pkg_check_modules(LIBDRM REQUIRED IMPORTED_TARGET libdrm)
pkg_check_modules(FONTCONFIG REQUIRED IMPORTED_TARGET fontconfig)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

add_compile_options(
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-unused-value
    -Wno-missing-field-initializers
    -Wpedantic
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    src/*.hpp
)

add_executable(hlmenu ${SOURCES})

target_include_directories(hlmenu PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(hlmenu 
    PkgConfig::HYPRTOOLKIT
    PkgConfig::HYPRUTILS
    PkgConfig::AQUAMARINE
    PkgConfig::HYPRGRAPHICS
    PkgConfig::PIXMAN
    PkgConfig::LIBDRM
    PkgConfig::FONTCONFIG
    PkgConfig::CAIRO
    PkgConfig::PANGO
    PkgConfig::PANGOCAIRO
    PkgConfig::GTK3
    PkgConfig::XKBCOMMON
    stdc++fs
    pthread
    dl
    m
)

include(GNUInstallDirs)
install(TARGETS hlmenu DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES resources/hlmenu.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
