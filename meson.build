project('capnp-c', ['c','cpp'], meson_version: '>=1.0.0', default_options : ['c_std=c99', 'cpp_std=c++14'])

cc = meson.get_compiler('c')

common_c_args = []
common_cpp_args = ['-std=c++14']
libcapnp_c_args = []
libcapnp_src = [
  'lib' / 'capn-malloc.c',
  'lib' / 'capn-stream.c',
  'lib' / 'capn.c',
]

libcapnp = library('capnp', libcapnp_src,
    c_args : common_c_args + libcapnp_c_args,
    dependencies: [],
    implicit_include_directories: false,
    include_directories: include_directories(['compiler', 'lib'])
)

libcapnp_dep = declare_dependency(
    link_with: libcapnp,
    dependencies: [],
    include_directories: include_directories(['compiler', 'lib'])
)

capnpc_src = [
  'compiler' / 'capnpc-c.c',
  'compiler' / 'codegen.c',
  'compiler' / 'codegen_codec.c',
  'compiler' / 'ctx.c',
  'compiler' / 'resolve.c',
  'compiler' / 'schema.capnp.c',
  'compiler' / 'str.c'
]

capnpc_c = executable('capnpc-c', capnpc_src,
      include_directories: include_directories(['lib']),
      dependencies: [libcapnp_dep],
      c_args : common_c_args,
)

capn_test_src = [
  'tests' / 'capn-test.cpp',
  'tests' / 'capn-stream-test.cpp',
  'tests' / 'example-test.cpp',
  'tests' / 'addressbook.capnp.c',
  'compiler' / 'test.capnp.c',
  'compiler' / 'schema-test.cpp',
  'compiler' / 'schema.capnp.c'
]

if get_option('enable_tests') and not meson.is_subproject()

  thread_dep = dependency('threads')

  gtest_proj = subproject('gtest')
  gtest_dep = gtest_proj.get_variable('gtest_dep')
  gmock_dep = gtest_proj.get_variable('gmock_dep')

  test_dependencies = [libcapnp_dep, gtest_dep, gmock_dep, thread_dep]

  if get_option('b_sanitize').contains('address')
    test_dependencies += cc.find_library('asan')
  endif
  if get_option('b_sanitize').contains('undefined')
    test_dependencies += cc.find_library('ubsan')
  endif

  exe = executable('capn-test', capn_test_src,
      include_directories: include_directories(['lib', 'tests', 'compiler']),
      dependencies: test_dependencies,
      c_args : common_c_args,
      cpp_args : common_cpp_args,
      install: false,
      install_rpath: '',
      implicit_include_directories: false
    )
  test('capn-test', exe)

  # data-codec test: generate schema code and test Data/List(Data) codec
  capnp = find_program('capnp', required: false)
  if capnp.found()
    data_codec_gen = custom_target('data-codec-capnp',
      input: 'tests' / 'data-codec.capnp',
      output: ['data-codec.capnp.c', 'data-codec.capnp.h'],
      command: [capnp, 'compile',
        '-o', capnpc_c.full_path() + ':' + '@OUTDIR@',
        '-I', meson.current_source_dir() / 'compiler',
        '--src-prefix', meson.current_source_dir() / 'tests',
        '@INPUT@'],
      depends: [capnpc_c],
    )

    gtest_main_dep = gtest_proj.get_variable('gtest_main_dep')
    data_codec_test_deps = [libcapnp_dep, gtest_dep, gtest_main_dep, thread_dep]

    data_codec_test_exe = executable('data-codec-test',
        'tests' / 'data-codec-test.cpp',
        data_codec_gen,
        include_directories: include_directories(['lib', 'tests', 'compiler']),
        dependencies: data_codec_test_deps,
        c_args : common_c_args + ['-D_POSIX_C_SOURCE=200809L'],
        cpp_args : common_cpp_args,
        install: false,
        implicit_include_directories: true
    )
    test('data-codec-test', data_codec_test_exe)
  endif

endif
