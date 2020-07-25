cmake_minimum_required(VERSION 3.0)
project(ReplaySorcery VERSION 0.2.2)
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
   src/mp4.c
   src/output.c
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

find_package(Threads REQUIRED)
target_link_libraries(${binary} PRIVATE ${CMAKE_THREAD_LIBS_INIT})

find_package(X11 REQUIRED)
if (NOT X11_FOUND OR NOT X11_Xutil_FOUND OR NOT X11_XShm_FOUND)
   message(SEND_ERROR "Failed to find X11 with util and shm extensions")
endif()
target_include_directories(${binary} SYSTEM PRIVATE ${X11_INCLUDE_DIRS})
target_link_libraries(${binary} PRIVATE ${X11_LIBRARIES})

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

target_include_directories(${binary} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dep/minimp4")
set_property(SOURCE src/mp4.c PROPERTY COMPILE_FLAGS -w)

add_custom_target(clang-format
   COMMAND clang-format -i ${sources}
   SOURCES ${sources}
   WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

install(TARGETS ${binary} DESTINATION bin)
set(service replay-sorcery.service)
configure_file(${service}.in ${service})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${service}" DESTINATION /usr/lib/systemd/user)
