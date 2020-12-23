cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.4.0)
include(CheckCCompilerFlag)
include(CheckIncludeFile)
include(ExternalProject)
find_package(PkgConfig REQUIRED)

set(binary replay-sorcery)
set(sources
   src/buffer.c
   src/config.c
   src/log.c
   src/main.c
   src/output.c
   src/util.c
   src/audio/aacenc.c
   src/audio/abuffer.c
   src/audio/adevice.c
   src/audio/aencoder.c
   src/audio/audio.c
   src/audio/fdkenc.c
   src/audio/pulsedev.c
   src/command/kmscmd.c
   src/control/control.c
   src/control/dbgctrl.c
   src/control/x11ctrl.c
   src/device/device.c
   src/device/ffdev.c
   src/device/kmsdev.c
   src/device/x11dev.c
   src/encoder/encoder.c
   src/encoder/ffenc.c
   src/encoder/openh264enc.c
   src/encoder/vaapienc.c
   src/encoder/x264enc.c
)
set(headers
   src/buffer.h
   src/config.h
   src/log.h
   src/output.h
   src/rsbuild.h.in
   src/util.h
   src/audio/abuffer.h
   src/audio/adevice.h
   src/audio/aencoder.h
   src/audio/audio.h
   src/command/kmscmd.h
   src/control/control.h
   src/device/device.h
   src/device/ffdev.h
   src/encoder/encoder.h
   src/encoder/ffenc.h
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
   target_c_flag(${binary} -Werror HAVE_ERROR_WARN)
endif()
option(RS_PROFILE "Enable code profiling" OFF)
if (RS_PROFILE)
   target_c_flag(${binary} -pg HAVE_PG_FLAG)
endif()

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

# FFmpeg
pkg_check_modules(FFMPEG
   REQUIRED IMPORTED_TARGET
   libavutil
   libavformat
   libavdevice
   libavcodec
   libavfilter
)
target_link_libraries(${binary} PRIVATE PkgConfig::FFMPEG)

# PThreads
find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
   set(RS_BUILD_PTHREAD_FOUND ON)
   target_link_libraries(${binary} PRIVATE Threads::Threads)
endif()

# X11
find_package(X11)
if (X11_FOUND)
   set(RS_BUILD_X11_FOUND ON)
   target_include_directories(${binary} SYSTEM PRIVATE ${X11_INCLUDE_DIRS})
   target_link_libraries(${binary} PRIVATE ${X11_LIBRARIES})
endif()

# PulseAudio
pkg_check_modules(PULSE IMPORTED_TARGET libpulse)
if (PULSE_FOUND)
   set(RS_BUILD_PULSE_FOUND ON)
   target_link_libraries(${binary} PRIVATE PkgConfig::PULSE)
endif()

# libdrm
pkg_check_modules(LIBDRM IMPORTED_TARGET libdrm)
if (LIBDRM_FOUND)
   set(RS_BUILD_LIBDRM_FOUND ON)
   target_link_libraries(${binary} PRIVATE PkgConfig::LIBDRM)
endif()

# Build configuration
check_include_file(unistd.h RS_BUILD_UNISTD_FOUND)
check_include_file(fcntl.h RS_BUILD_FCNTL_FOUND)
configure_file(src/rsbuild.h.in rsbuild.h)
target_include_directories(${binary} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Clang format target to make formatting easy
add_custom_target(clang-format
   COMMAND clang-format -i ${sources} ${headers}
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Install binary
option(RS_SETID "Enable the SETUID and SETGID permissions" ON)
set(permissions
   OWNER_READ OWNER_WRITE OWNER_EXECUTE
   GROUP_READ GROUP_EXECUTE
   WORLD_READ WORLD_EXECUTE
)
if (RS_SETID)
   set(permissions ${permissions} SETUID SETGID)
endif()
install(TARGETS ${binary} DESTINATION bin PERMISSIONS ${permissions})

# Install config
install(FILES sys/replay-sorcery.conf DESTINATION etc)

# Install service
set(RS_SYSTEMD_DIR /usr/lib/systemd/user CACHE STRING "Where to install the systemd service")
set(service replay-sorcery.service)
configure_file(sys/${service}.in ${service})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${service}" DESTINATION "${RS_SYSTEMD_DIR}")
