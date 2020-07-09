project('ReplaySorcery', 'c', version: '0.1.0-dev', default_options: [
   'c_std=c99',
   'warning_level=3'
])
compiler = meson.get_compiler('c')

unwind = dependency('libunwind', include_type: 'system', required: false)
configure_file(
   output: 'rs-build-config.h',
   configuration: {
      'RS_CONFIG_UNWIND': unwind.found(),
      'RS_CONFIG_X264': get_option('x264-gpl')
   }
)

executable(
   'replay-sorcery', [
      'src/main.c',
      'src/error.c',
      'src/path.c',
      'src/pktcircle.c',
      'src/record.c',
      'src/save.c',
      'src/config/config.c',
      'src/config/file.c',
      'src/config/paths.c',
      'src/input/input.c',
      'src/input/video.c',
      'src/encoder/encoder.c',
      'src/encoder/video.c',
      'src/user/user.c',
      'src/user/debug.c',
      'src/user/x11.c',
      'src/user/default.c'
   ],
   c_args: compiler.get_supported_arguments([
      '-Wconversion',
      '-Wstrict-prototypes',
      '-Wmissing-prototypes'
   ]),
   dependencies: [
      unwind,
      dependency('threads', include_type: 'system'),

      dependency('libavutil', include_type: 'system'),
      dependency('libavformat', include_type: 'system'),
      dependency('libavdevice', include_type: 'system'),
      dependency('libavcodec', include_type: 'system'),
      dependency('libswscale', include_type: 'system'),

      dependency('x11', include_type: 'system')
   ]
)
