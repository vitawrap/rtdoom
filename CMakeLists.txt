cmake_minimum_required(VERSION 3.10)
project(rtdoom LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# specifically to help raylib build
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0077 NEW)

# release config
add_compile_options(
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-O3>"
    #"$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-ffast-math>"
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-fno-stack-protector>"
    #"$<$<CONFIG:Debug>:-fsanitize=address>"
    #"$<$<CONFIG:Debug>:-O1>"
    #"$<$<CONFIG:Debug>:-fno-omit-frame-pointer>"
)

add_link_options(
    #"$<$<CONFIG:Debug>:-fsanitize=address>"
)

include(FetchContent)

if (NOT (CMAKE_TOOLCHAIN_FILE MATCHES ".*Emscripten\.cmake$"))
  # force older dwarf debug info
  add_compile_options(-gdwarf-4)

  FetchContent_Declare(
    sdl2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-2.30.11 # latest release: jan 2026
  )
  # note about SDL2 on windows: the build will generate a windows.rc.res.res file instead of windows.rc.res
  # it has to be renamed to the latter to function properly...

  # macro(add_custom_target _target)
  #     if(NOT "${_target}" STREQUAL "uninstall")
  #         _add_custom_target(${ARGV})
  #     endif()
  # endmacro()

  FetchContent_MakeAvailable(sdl2)
endif ()

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # latest release: dec 2025
)

FetchContent_MakeAvailable(glm)

file(GLOB_RECURSE SOURCES
    "rtdoom/*.cpp"
    "rtdoom/*.c"
)

# main executable

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE rtdoom include)
target_include_directories(${PROJECT_NAME} PUBLIC "${SDL2_SOURCE_DIR}/include" "${glm_SOURCE_DIR}")

if (CMAKE_TOOLCHAIN_FILE MATCHES ".*Emscripten\.cmake$")
  target_link_libraries(${PROJECT_NAME}
    PRIVATE glm
  )
  target_compile_options(${PROJECT_NAME} PRIVATE -sUSE_SDL=2 "$<$<CONFIG:Debug>:-g>")
  target_link_options(${PROJECT_NAME} PRIVATE
    -sUSE_SDL=2
    "$<$<CONFIG:Debug>:-sASSERTIONS=1>"
    -sRUNTIME_DEBUG=0
    -sLIBRARY_DEBUG=0
    -sMAX_WEBGL_VERSION=2
    "$<$<CONFIG:Debug>:--emit-symbol-map>"
    --embed-file "${CMAKE_SOURCE_DIR}/asset-web@/")
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
else()
  target_link_libraries(${PROJECT_NAME}
    PRIVATE glm SDL2::SDL2main SDL2::SDL2-static
  )
endif()

# link-time linux specific shenanigans
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m pthread dl rt)
endif()
