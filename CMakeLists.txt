cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.2.0)
include(CheckCCompilerFlag)

set(binary replay-sorcery)
set(sources
   src/util/log.c
   src/util/memory.c
   src/util/buffer.c
   src/util/circle.c
   src/util/frame.c
   src/system/system.c
   src/system/framerate.c
   src/system/xlib.c
   src/config/config.c
   src/compress.c
   src/main.c
)

add_executable(${binary} ${sources})
set_property(TARGET ${binary} PROPERTY C_STANDARD 99)

function(target_c_flag target flag)
   message("Checking '${flag}'...")
   unset(c_flag_compiles CACHE)
   check_c_compiler_flag(${flag} c_flag_compiles)
   if (c_flag_compiles)
      target_compile_options(${target} PRIVATE ${flag})
   endif()
endfunction()
target_c_flag(${binary} -Wall)
target_c_flag(${binary} -Wextra)
target_c_flag(${binary} -Wpedantic)
target_c_flag(${binary} -Wconversion)
target_c_flag(${binary} -Wstrict-prototypes)
target_c_flag(${binary} -Wmissing-prototypes)

find_package(X11 REQUIRED)
target_include_directories(${binary} SYSTEM PRIVATE ${X11_INCLUDE_DIRS})
target_link_libraries(${binary} PRIVATE ${X11_LIBRARIES})

set(ENABLE_SHARED OFF CACHE BOOL "")
set(WITH_TURBOJPEG OFF CACHE BOOL "")
add_subdirectory(dep/libjpeg-turbo)
target_link_libraries(${binary} PRIVATE jpeg-static)

# set(ENABLE_WEBMINFO OFF CACHE BOOL "")
# set(ENABLE_WEBMTS OFF CACHE BOOL "")
# add_subdirectory(dep/libwebm)
# target_link_libraries(${binary} PRIVATE webm)

add_custom_target(clang-format
   COMMAND clang-format -i ${sources}
   SOURCES ${sources}
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
