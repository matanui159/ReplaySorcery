cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.2.0)
include(CheckCCompilerFlag)
find_package(PkgConfig REQUIRED)
set(name replay-sorcery)

add_executable(${name}
   src/common.c
   src/config/config.c
   src/system/system.c
   src/system/framerate.c
   src/system/xlib.c
   src/buffer.c
   src/circle.c
   src/compress.c
   src/main.c
)
set_property(TARGET ${name} PROPERTY C_STANDARD 99)

function(target_c_flag target flag)
   message("-- Checking '${flag}'...")
   unset(c_flag_compiles CACHE)
   check_c_compiler_flag(${flag} c_flag_compiles)
   if (c_flag_compiles)
      target_compile_options(${target} PRIVATE ${flag})
   endif()
endfunction()
target_c_flag(${name} -Wall)
target_c_flag(${name} -Wextra)
target_c_flag(${name} -Wpedantic)
target_c_flag(${name} -Wconversion)
target_c_flag(${name} -Wstrict-prototypes)
target_c_flag(${name} -Wmissing-prototypes)

pkg_check_modules(UNWIND IMPORTED_TARGET libunwind)
if (UNWIND_FOUND)
   target_link_libraries(${name} PRIVATE PkgConfig::UNWIND)
endif()

find_package(X11 REQUIRED)
target_include_directories(${name} PRIVATE ${X11_INCLUDE_DIRS})
target_link_libraries(${name} PRIVATE ${X11_LIBRARIES})

set(ENABLE_SHARED OFF CACHE BOOL "")
set(WITH_TURBOJPEG OFF CACHE BOOL "")
add_subdirectory(dep/libjpeg-turbo)
target_link_libraries(${name} PRIVATE jpeg-static)

set(ENABLE_WEBMINFO OFF CACHE BOOL "")
set(ENABLE_WEBMTS OFF CACHE BOOL "")
add_subdirectory(dep/libwebm)
target_link_libraries(${name} PRIVATE webm)

configure_file(rs-build-config.h.in rs-build-config.h)
target_include_directories(${name} PRIVATE "${PROJECT_BINARY_DIR}")
