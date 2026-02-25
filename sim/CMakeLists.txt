cmake_minimum_required(VERSION 3.21)
project(roundie_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# ---------------------------------------------------------------------------
# SDL2: prefer a system/vcpkg installation; fall back to FetchContent so that
# a plain clone-and-build works on Windows without any extra setup.
# ---------------------------------------------------------------------------
find_package(SDL2 CONFIG QUIET)
if(NOT SDL2_FOUND)
  message(STATUS "SDL2 not found via find_package – falling back to FetchContent.")
  message(STATUS "  On Windows with vcpkg, install sdl2 and pass:")
  message(STATUS "    -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake")
  message(STATUS "    -DVCPKG_TARGET_TRIPLET=x64-windows  (when building -A x64)")
  message(STATUS "SDL2 not found on this system – fetching from GitHub")
  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.11
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(SDL2)

  # SDL2's source tree places headers at  include/SDL.h  (no SDL2/ subdir).
  # LVGL's SDL driver #includes them as  SDL2/SDL.h  (installed-package layout).
  # Generate thin forwarding headers under a SDL2/ directory in the build tree
  # so that both spellings resolve to the same real file.
  set(_sdl2_fwd "${CMAKE_CURRENT_BINARY_DIR}/sdl2_fwd")
  file(MAKE_DIRECTORY "${_sdl2_fwd}/SDL2")
  file(GLOB _sdl2_hdrs "${sdl2_SOURCE_DIR}/include/*.h")
  foreach(_h ${_sdl2_hdrs})
    get_filename_component(_n "${_h}" NAME)
    file(WRITE "${_sdl2_fwd}/SDL2/${_n}" "#pragma once\n#include \"${_h}\"\n")
  endforeach()
endif()

FetchContent_Declare(
  lvgl
  GIT_REPOSITORY https://github.com/lvgl/lvgl.git
  GIT_TAG v9.1.0
)
# Must be set as a CACHE variable before FetchContent_MakeAvailable so LVGL
# picks it up and adds it as a PUBLIC compile definition for all consumers.
set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h" CACHE STRING "" FORCE)
# On Windows/MSVC build LVGL as a static library to avoid the PreLink
# "cmake -E __create_def" step that MSVC triggers when generating DLL export
# tables (error MSB3073).  LVGL's LV_EXPORT_API macros expand to nothing for
# static builds so there are no __declspec(dllimport) link errors.
# On Linux/macOS the default (shared) is kept so existing behaviour is unchanged.
if(MSVC)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
else()
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
endif()
FetchContent_MakeAvailable(lvgl)

# Explicitly add sim/ as a non-SYSTEM PUBLIC include so that
# #include "lv_conf.h" (used by LV_CONF_INCLUDE_SIMPLE) resolves to
# sim/lv_conf.h with higher priority than any SYSTEM include path.
target_include_directories(lvgl PUBLIC "${CMAKE_CURRENT_LIST_DIR}")

# LVGL 9.1.0 unconditionally appends
#   INTERFACE LV_ATTRIBUTE_EXTERN_DATA=__declspec(dllimport)
# for MSVC regardless of BUILD_SHARED_LIBS.  For a static build this causes
# every consumer of lvgl headers (e.g. roundie_sim) to emit __imp_ references
# that have no matching symbol in lvgl.lib, producing LNK2019 errors.
# Strip that definition from lvgl's INTERFACE for static (non-DLL) builds.
if(MSVC AND NOT BUILD_SHARED_LIBS)
  get_target_property(_lvgl_iface_defs lvgl INTERFACE_COMPILE_DEFINITIONS)
  if(_lvgl_iface_defs)
    list(FILTER _lvgl_iface_defs EXCLUDE REGEX "^LV_ATTRIBUTE_EXTERN_DATA")
    set_target_properties(lvgl PROPERTIES
      INTERFACE_COMPILE_DEFINITIONS "${_lvgl_iface_defs}")
  endif()
endif()

# LVGL's own SDL driver sources (#include "SDL2/SDL.h") need SDL2 headers.
# When SDL2 came from vcpkg/system, forward its interface include directories;
# when it came from FetchContent, use the thin forwarding-header layer.
if(SDL2_FOUND)
  target_include_directories(lvgl PRIVATE
    $<TARGET_PROPERTY:SDL2::SDL2,INTERFACE_INCLUDE_DIRECTORIES>)
elseif(DEFINED _sdl2_fwd)
  target_include_directories(lvgl PRIVATE "${_sdl2_fwd}")
endif()

add_executable(roundie_sim
  main.cpp
  sim_globals.cpp
)

target_include_directories(roundie_sim PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}          # sim/ first: config.h, Arduino.h, Preferences.h
  ${CMAKE_CURRENT_LIST_DIR}/../roundie
)

# When SDL2 was fetched (no system install), expose the SDL2/ forwarding-header
# layer so that #include "SDL2/SDL.h" resolves to the real SDL.h.
if(DEFINED _sdl2_fwd)
  target_include_directories(roundie_sim PRIVATE "${_sdl2_fwd}")
endif()

target_link_libraries(roundie_sim PRIVATE lvgl::lvgl SDL2::SDL2 SDL2::SDL2main)

# Compile LVGL's SDL driver sources into the exe
target_sources(roundie_sim PRIVATE
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_window.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_mouse.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_keyboard.c
  ${lvgl_SOURCE_DIR}/src/drivers/sdl/lv_sdl_mousewheel.c
)

target_compile_definitions(roundie_sim PRIVATE SDL_MAIN_HANDLED=1)
