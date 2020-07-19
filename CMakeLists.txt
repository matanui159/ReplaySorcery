cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.2.0)
include(CheckCCompilerFlag)
include(ExternalProject)

set(binary replay-sorcery)
set(sources
   src/util/log.c
   src/util/memory.c
   src/util/buffer.c
   src/util/circle.c
   src/util/path.c
   src/util/string.c
   src/util/frame.c
   src/system/system.c
   src/system/framerate.c
   src/system/xlib.c
   src/config.c
   src/compress.c
   src/main.c
)

add_executable(${binary} ${sources})
set_property(TARGET ${binary} PROPERTY C_STANDARD 99)

function(target_c_flag target flag var)
   check_c_compiler_flag(${flag} ${var})
   if (${var})
      target_compile_options(${target} PRIVATE ${flag})
      target_link_options(${target} PRIVATE ${flag})
   endif()
endfunction()
target_c_flag(${binary} -g HAVE_G_FLAG)
target_c_flag(${binary} -Wall HAVE_ALL_WARN)
target_c_flag(${binary} -Wextra HAVE_EXTRA_WARN)
target_c_flag(${binary} -Wpedantic HAVE_PEDANTIC_WARN)
target_c_flag(${binary} -Wconversion HAVE_CONVERSION_WARN)
target_c_flag(${binary} -Wstrict-prototypes HAVE_STRICT_PROTOTYPES_WARN)
target_c_flag(${binary} -Wmissing-prototypes HAVE_MISSING_PROTOTYPES_WARN)
option(RS_WARN_ERROR "Fail on compiler warning" OFF)
target_c_flag(${binary} -Werror RS_WARN_ERROR)
option(RS_PROFILE "Enable code profiling" OFF)
target_c_flag(${binary} -pg RS_PROFILE)

find_package(X11 REQUIRED)
if (NOT X11_FOUND OR NOT X11_Xutil_FOUND OR NOT X11_XShm_FOUND)
   message(SEND_ERROR "Failed to find X11 with util and shm extensions")
endif()
target_include_directories(${binary} SYSTEM PRIVATE ${X11_INCLUDE_DIRS})
target_link_libraries(${binary} PRIVATE ${X11_LIBRARIES})

ExternalProject_Add(backtrace
   STEP_TARGETS backtrace
   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep/libbacktrace"
   CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
)
add_dependencies(${binary} backtrace)
ExternalProject_Get_Property(backtrace INSTALL_DIR)
target_include_directories(${binary} SYSTEM PRIVATE "${INSTALL_DIR}/include")
target_link_libraries(${binary} PRIVATE "${INSTALL_DIR}/lib/libbacktrace.a")

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
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
