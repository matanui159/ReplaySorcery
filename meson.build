project('SourceReplay', 'c', default_options: [
   'c_std=c99',
   'warning_level=3'
])
compiler = meson.get_compiler('c')

unwind = dependency('libunwind', include_type: 'system', required: false)
configure_file(
   output: 'sr-config.h',
   configuration: {
      'SR_CONFIG_UNWIND': unwind.found(),
      'SR_CONFIG_X264': get_option('x264_gpl')
   }
)

executable(
   'replay', [
      'src/main.c',
      'src/error.c',
      'src/pktcircle.c',
      'src/input/input.c',
      'src/input/video.c',
      'src/encoder/encoder.c',
      'src/encoder/video.c',
      'src/record.c',
      'src/user/user.c',
      'src/user/debug.c',
      'src/user/default.c',
      'src/save.c'
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
      dependency('libswscale', include_type: 'system')
   ]
)
