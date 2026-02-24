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
FetchContent_MakeAvailable(lvgl)

add_executable(roundie_sim
  main.cpp
  sim_globals.cpp
)

target_include_directories(roundie_sim PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}          # sim/ (Preferences.h stub)
  ${CMAKE_CURRENT_LIST_DIR}/../roundie
)

# Generate a tiny lv_conf.h for the simulator
file(WRITE "${CMAKE_BINARY_DIR}/lv_conf.h" "
#pragma once
#define LV_USE_OS 0
#define LV_COLOR_DEPTH 16
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
")
target_include_directories(roundie_sim PRIVATE ${CMAKE_BINARY_DIR})
target_compile_definitions(roundie_sim PRIVATE LV_CONF_INCLUDE_SIMPLE=1)

target_link_libraries(roundie_sim PRIVATE lvgl::lvgl SDL2::SDL2 SDL2::SDL2main)

# Compile LVGL's SDL driver sources into the exe
target_sources(roundie_sim PRIVATE
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_window.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_mouse.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_keyboard.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_mousewheel.c
)

target_compile_definitions(roundie_sim PRIVATE LV_USE_SDL=1 SDL_MAIN_HANDLED=1)