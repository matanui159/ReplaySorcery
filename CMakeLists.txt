cmake_minimum_required(VERSION 3.13)
project(ReplaySorcery VERSION 0.5.1)
include(CheckCCompilerFlag)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(ExternalProject)
find_package(PkgConfig REQUIRED)

set(binary replay-sorcery)
set(sources
   src/buffer.c
   src/config.c
   src/log.c
   src/main.c
   src/output.c
   src/socket.c
   src/thread.c
   src/util.c
   src/audio/aacenc.c
   src/audio/abuffer.c
   src/audio/adevice.c
   src/audio/aencoder.c
   src/audio/audio.c
   src/audio/fdkenc.c
   src/audio/pulsedev.c
   src/command/ctrlcmd.c
   src/command/kmscmd.c
   src/command/svkmscmd.c
   src/control/cmdctrl.c
   src/control/control.c
   src/control/dbgctrl.c
   src/control/x11ctrl.c
   src/device/device.c
   src/device/ffdev.c
   src/device/kmsdev.c
   src/device/svkmsdev.c
   src/device/x11dev.c
   src/encoder/encoder.c
   src/encoder/ffenc.c
   src/encoder/openh264enc.c
   src/encoder/vah264enc.c
   src/encoder/vahevcenc.c
   src/encoder/x264enc.c
   src/encoder/x265enc.c
)
set(headers
   src/buffer.h
   src/config.h
   src/log.h
   src/output.h
   src/rsbuild.h.in
   src/socket.h
   src/thread.h
   src/util.h
   src/audio/abuffer.h
   src/audio/adevice.h
   src/audio/aencoder.h
   src/audio/audio.h
   src/command/command.h
   src/control/control.h
   src/device/device.h
   src/device/x11dev.h
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
target_c_flag(${binary} -Wvla HAVE_VLA_WARN)

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
      --includedir=<INSTALL_DIR>/include
      --libdir=<INSTALL_DIR>/lib
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

# Posix I/O
check_symbol_exists(open fcntl.h POSIX_OPEN_FOUND)
check_symbol_exists(close unistd.h POSIX_CLOSE_FOUND)
check_symbol_exists(read unistd.h POSIX_READ_FOUND)
check_symbol_exists(write unistd.h POSIX_WRITE_FOUND)
check_symbol_exists(fcntl fcntl.h POSIX_FCNTL_FOUND)
check_symbol_exists(umask sys/stat.h POSIX_UMASK_FOUND)
check_symbol_exists(mkdir sys/stat.h POSIX_MKDIR_FOUND)
check_symbol_exists(poll poll.h POSIX_POLL_FOUND)
if (
   POSIX_OPEN_FOUND AND
   POSIX_CLOSE_FOUND AND
   POSIX_READ_FOUND AND
   POSIX_WRITE_FOUND AND
   POSIX_FCNTL_FOUND AND
   POSIX_UMASK_FOUND AND
   POSIX_MKDIR_FOUND AND
   POSIX_POLL_FOUND
)
   set(RS_BUILD_POSIX_IO_FOUND ON)
endif()

# Unix Sockets
check_symbol_exists(socket sys/socket.h UNIX_SOCKET_FOUND)
check_symbol_exists(bind sys/socket.h UNIX_BIND_FOUND)
check_symbol_exists(listen sys/socket.h UNIX_LISTEN_FOUND)
check_symbol_exists(accept sys/socket.h UNIX_ACCEPT_FOUND)
check_symbol_exists(connect sys/socket.h UNIX_CONNECT_FOUND)
check_symbol_exists(sendmsg sys/socket.h UNIX_SENDMSG_FOUND)
check_symbol_exists(recvmsg sys/socket.h UNIX_RECVMSG_FOUND)
check_include_file(sys/un.h UNIX_UN_FOUND)
if (
   RS_BUILD_POSIX_IO_FOUND AND
   UNIX_SOCKET_FOUND AND
   UNIX_BIND_FOUND AND
   UNIX_LISTEN_FOUND AND
   UNIX_ACCEPT_FOUND AND
   UNIX_CONNECT_FOUND AND
   UNIX_SENDMSG_FOUND AND
   UNIX_RECVMSG_FOUND AND
   UNIX_UN_FOUND
)
   set(RS_BUILD_UNIX_SOCKET_FOUND ON)
endif()

# PThreads
find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
   set(RS_BUILD_PTHREAD_FOUND ON)
   target_link_libraries(${binary} PRIVATE Threads::Threads)
endif()

# X11
find_package(X11)
if (X11_FOUND AND X11_xcb_FOUND)
   set(RS_BUILD_X11_FOUND ON)
   target_include_directories(${binary} SYSTEM PRIVATE ${X11_INCLUDE_DIRS} ${X11_xcb_INCLUDE_PATH})
   target_link_libraries(${binary} PRIVATE ${X11_LIBRARIES} ${X11_xcb_LIB})
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
configure_file(src/rsbuild.h.in rsbuild.h)
target_include_directories(${binary} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Clang format target to make formatting easy
add_custom_target(clang-format
   COMMAND clang-format -i ${sources} ${headers}
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Install binary
install(TARGETS ${binary} DESTINATION bin PERMISSIONS ${permissions})

# Install config
install(FILES sys/replay-sorcery.conf DESTINATION etc)

# Install services
set(RS_SYSTEMD_DIR /usr/lib/systemd CACHE STRING "Where to install the systemd services")
configure_file(sys/replay-sorcery.service.in replay-sorcery.service)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/replay-sorcery.service" DESTINATION "${RS_SYSTEMD_DIR}/user")
configure_file(sys/replay-sorcery-kms.service.in replay-sorcery-kms.service)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/replay-sorcery-kms.service" DESTINATION "${RS_SYSTEMD_DIR}/system")
