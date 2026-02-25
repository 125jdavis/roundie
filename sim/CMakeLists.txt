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
# tables (error MSB3073).  LVGL's own CMakeLists.txt still propagates
# LV_ATTRIBUTE_EXTERN_DATA=__declspec(dllimport) as an INTERFACE definition
# even for static builds; we strip that below after FetchContent_MakeAvailable.
# On Linux/macOS the default (shared) is kept so existing behaviour is unchanged.
if(MSVC)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
else()
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
endif()
FetchContent_MakeAvailable(lvgl)

# LVGL's CMakeLists.txt unconditionally propagates
# LV_ATTRIBUTE_EXTERN_DATA=__declspec(dllimport) to all consumers via the
# INTERFACE_COMPILE_DEFINITIONS of the lvgl target, regardless of whether
# LVGL was built as a shared or static library.  For a static build (which we
# force on MSVC above) this causes LNK2019 errors because the linker looks for
# __imp_ symbols that only exist in DLL import libraries.  Strip the
# dllimport annotation from the interface so that consumers see plain extern.
if(MSVC)
  get_target_property(_lvgl_iface_defs lvgl INTERFACE_COMPILE_DEFINITIONS)
  if(_lvgl_iface_defs)
    list(REMOVE_ITEM _lvgl_iface_defs
      "LV_ATTRIBUTE_EXTERN_DATA=__declspec(dllimport)")
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

# Copy SDL2.dll next to the executable so the artifact is self-contained.
# Only needed on Windows when SDL2 was fetched (not a system/vcpkg install).
if(WIN32 AND NOT SDL2_FOUND)
  add_custom_command(TARGET roundie_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL2::SDL2>
      $<TARGET_FILE_DIR:roundie_sim>
    COMMENT "Copying SDL2.dll to output directory"
  )
endif()
