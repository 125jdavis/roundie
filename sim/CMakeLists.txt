cmake_minimum_required(VERSION 3.21)
project(roundie_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v9.1.0
)
# Must be set as a CACHE variable before FetchContent_MakeAvailable so LVGL
# picks it up and adds it as a PUBLIC compile definition for all consumers.
set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE STRING "" FORCE)
# LVGL v9 uses __declspec(dllimport) for extern data on MSVC, which requires a
# shared library.  Build LVGL as a shared lib (DLL on Windows) so the import
# symbols exist at link time.
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
FetchContent_MakeAvailable(lvgl)

add_executable(roundie_sim
  main.cpp
  sim_globals.cpp
)

target_include_directories(roundie_sim PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}          # sim/ first: config.h, Arduino.h, Preferences.h
  ${CMAKE_CURRENT_LIST_DIR}/../roundie
)

target_link_libraries(roundie_sim PRIVATE lvgl::lvgl SDL2::SDL2 SDL2::SDL2main)

# Compile LVGL's SDL driver sources into the exe
target_sources(roundie_sim PRIVATE
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_window.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_mouse.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_keyboard.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_mousewheel.c
)

target_compile_definitions(roundie_sim PRIVATE SDL_MAIN_HANDLED=1)
