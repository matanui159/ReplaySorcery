cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.2.6)
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
   src/audio.c
   src/config.c
   src/compress.c
   src/mp4.c
   src/output.c
   src/main.c
)
set(headers
   src/std.h
   src/util/log.h
   src/util/memory.h
   src/util/buffer.h
   src/util/circle.h
   src/util/path.h
   src/util/string.h
   src/util/frame.h
   src/system/system.h
   src/system/framerate.h
   src/system/xlib.h
   src/audio.h
   src/config.h
   src/compress.h
   src/output.h
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
target_link_libraries(${binary} PRIVATE ${CMAKE_THREAD_LIBS_INIT})

# X11 with Xutil and MIT-SHM
find_package(X11 REQUIRED)
if (NOT X11_FOUND OR NOT X11_Xutil_FOUND OR NOT X11_XShm_FOUND)
   message(SEND_ERROR "Failed to find X11 with util and shm extensions")
endif()
target_include_directories(${binary} SYSTEM PRIVATE ${X11_INCLUDE_DIRS})
target_link_libraries(${binary} PRIVATE ${X11_LIBRARIES})

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

# x264 git submodule
ExternalProject_Add(x264
   STEP_TARGETS x264
   SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep/x264"
   CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --prefix=<INSTALL_DIR>
      --enable-static
      --disable-cli
      --disable-opencl
      --extra-cflags=-O3
)
add_dependencies(${binary} x264)
ExternalProject_Get_Property(x264 INSTALL_DIR)
target_include_directories(${binary} SYSTEM PRIVATE "${INSTALL_DIR}/include")
target_link_libraries(${binary} PRIVATE "${INSTALL_DIR}/lib/libx264.a" m)

# sdl2 git submodule
ExternalProject_Add(sdl2
	STEP_TARGETS sdl2
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep/sdl2"
	CONFIGURE_COMMAND <SOURCE_DIR>/configure
	   --prefix=<INSTALL_DIR>
	   --enable-static=yes
	   --enable-shared=no
	   --enable-video=no
	   --enable-render=no
	   --enable-joystick=no
	   --enable-haptic=no
	   --enable-sensor=no
	   --enable-power=no
	   --enable-filesystem=no
	   --enable-timers=no
	   --enable-file=no
	   --enable-audio=yes
)
add_dependencies(${binary} sdl2)
ExternalProject_Get_Property(sdl2 INSTALL_DIR)
target_include_directories(${binary} SYSTEM PRIVATE "${INSTALL_DIR}/include")
target_link_libraries(${binary} PRIVATE "${INSTALL_DIR}/lib/libSDL2.a" dl)

# fdk-aac git submodule
ExternalProject_Add(fdk-aac
	STEP_TARGETS fdk-aac
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dep/fdk-aac"
	CONFIGURE_COMMAND cd <SOURCE_DIR> && ./autogen.sh && cd <BINARY_DIR> && <SOURCE_DIR>/configure
	   --prefix=<INSTALL_DIR>
	   --enable-static=yes
	   --enable-shared=no
)
add_dependencies(${binary} fdk-aac)
ExternalProject_Get_Property(fdk-aac INSTALL_DIR)
target_include_directories(${binary} SYSTEM PRIVATE "${INSTALL_DIR}/include")
target_link_libraries(${binary} PRIVATE "${INSTALL_DIR}/lib/libfdk-aac.a")

# Minimp4 header-only git submodule
target_include_directories(${binary} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dep/minimp4")
set_property(SOURCE src/mp4.c PROPERTY COMPILE_FLAGS -w)

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
