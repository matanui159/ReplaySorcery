project('ReplaySorcery', 'c', version: '0.2.0-dev', default_options: [
   'c_std=c99',
   'warning_level=3'
])
cmake = import('cmake')
compiler = meson.get_compiler('c')

unwind = dependency('libunwind', include_type: 'system', required: false)
configure_file(
   output: 'rs-build-config.h',
   configuration: {
      'RS_CONFIG_LIB_UNWIND': unwind.found(),
      'RS_CONFIG_ATTR_NORETURN': compiler.has_function_attribute('noreturn')
   }
)

jpegOpts = cmake.subproject_options()
jpegOpts.set_override_option('warning_level', '0')
jpegOpts.add_cmake_defines({
   'ENABLE_SHARED': false,
   'WITH_SIMD': false,
   'WITH_TURBOJPEG': false
})
jpeg = cmake.subproject('libjpeg-turbo', options: jpegOpts).dependency('jpeg-static')

webmOpts = cmake.subproject_options()
webmOpts.set_override_option('warning_level', '0')
webm = cmake.subproject('libwebm', options: webmOpts).dependency('webm')

executable(
   'replay-sorcery', [
      'src/common.c',
      'src/config/config.c',
      'src/system/system.c',
      'src/system/framerate.c',
      'src/system/xlib.c',
      'src/buffer.c',
      'src/circle.c',
      'src/compress.c',
      'src/main.c'
   ],
   c_args: compiler.get_supported_arguments([
      '-D_DEFAULT_SOURCE',
      '-Wconversion',
      '-Wstrict-prototypes',
      '-Wmissing-prototypes'
   ]),
   dependencies: [
      unwind,
      jpeg,
      webm,
      dependency('threads', include_type: 'system'),
      dependency('x11', include_type: 'system'),
      dependency('xext', include_type: 'system')
   ]
)
