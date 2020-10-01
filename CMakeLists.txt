cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.4.0)
include(CheckCCompilerFlag)
include(ExternalProject)
find_package(PkgConfig REQUIRED)

set(binary replay-sorcery)
set(sources
   src/main.c
   src/util/log.c
   src/device/demux.c
)
set(headers
   src/util/log.h
   src/device/device.h
   src/device/demux.h
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
# We always need debug info for libbacktrace
target_c_flag(${binary} -g HAVE_G_FLAG)
# Check for various warning flags
target_c_flag(${binary} -Wall HAVE_ALL_WARN)
target_c_flag(${binary} -Wextra HAVE_EXTRA_WARN)
target_c_flag(${binary} -Wpedantic HAVE_PEDANTIC_WARN)
target_c_flag(${binary} -Wconversion HAVE_CONVERSION_WARN)
target_c_flag(${binary} -Wstrict-prototypes HAVE_STRICT_PROTOTYPES_WARN)
target_c_flag(${binary} -Wmissing-prototypes HAVE_MISSING_PROTOTYPES_WARN)

# Used for CI. I do not like warnings in my code but during development its fine
option(RS_WARN_ERROR "Fail on compiler warning" OFF)
if (RS_WARN_ERROR)
   target_c_flag(${binary} -Werror HAVE_WARN_ERROR)
endif()

option(RS_PROFILE "Enable code profiling" OFF)
if (RS_PROFILE)
   target_c_flag(${binary} -pg HAVE_PG_FLAG)
endif()

# Pthreads
find_package(Threads REQUIRED)
target_link_libraries(${binary} PRIVATE Threads::Threads)

# FFmpeg
pkg_check_modules(FFMPEG
   REQUIRED IMPORTED_TARGET
   libavutil
   libavformat
   libavcodec
   libavdevice
   libavfilter
)
target_link_libraries(${binary} PRIVATE PkgConfig::FFMPEG)

# libbacktrace git submodule
ExternalProject_Add(backtrace
   STEP_TARGETS backtrace
   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep/libbacktrace"
   CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
      CFLAGS=-O3
)
add_dependencies(${binary} backtrace)
ExternalProject_Get_Property(backtrace INSTALL_DIR)
target_include_directories(${binary} SYSTEM PRIVATE "${INSTALL_DIR}/include")
target_link_libraries(${binary} PRIVATE "${INSTALL_DIR}/lib/libbacktrace.a")

# libjpeg-turbo git submodule
ExternalProject_Add(jpeg
   STEP_TARGETS jpeg
   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep/libjpeg-turbo"
   CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
      -DCMAKE_INSTALL_LIBDIR=<INSTALL_DIR>/lib
      -DENABLE_SHARED=OFF
      -DWITH_TURBOJPEG=OFF
      -DCMAKE_C_FLAGS=-O3
)
add_dependencies(${binary} jpeg)
ExternalProject_Get_Property(jpeg INSTALL_DIR)
target_include_directories(${binary} SYSTEM PRIVATE "${INSTALL_DIR}/include")
target_link_libraries(${binary} PRIVATE "${INSTALL_DIR}/lib/libjpeg.a")

# Clang format target to make formatting easy
add_custom_target(clang-format
   COMMAND clang-format -i ${sources} ${headers}
   SOURCES ${sources}
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Installation
install(TARGETS ${binary} DESTINATION bin)
set(service replay-sorcery.service)
# We cannot use CMAKE_INSTALL_PREFIX since the default is /usr/local
set(RS_SYSTEMD_DIR /usr/lib/systemd/user CACHE STRING "Where to install the systemd service")
configure_file(${service}.in ${service})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${service}" DESTINATION "${RS_SYSTEMD_DIR}")
# Similar for here since we need to use the XDG path /etc/xdg
set(RS_CONFIG_DIR /etc/xdg CACHE STRING "Where to install the default config")
install(FILES replay-sorcery.default.conf DESTINATION "${RS_CONFIG_DIR}" RENAME replay-sorcery.conf)
