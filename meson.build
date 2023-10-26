project('snitch', 'cpp',
  default_options: ['cpp_std=c++20', 'default_library=static'],
  version: '1.2.3'
)

cxx = meson.get_compiler('cpp')
compiler_id = cxx.get_id()
host_system = host_machine.system()
is_mingw = host_system == 'windows' and cxx.get_id() == 'gcc'

if get_option('default_library') == 'shared'
  if compiler_id == 'msvc' or is_mingw
    # Nothing to do; default is already to hide symbols unless exported.
  elif compiler_id == 'gcc' or compiler_id == 'clang'
    # Set default visibility to "hidden" so only exported symbols are visible.
    add_project_arguments(
      cxx.get_supported_arguments([
        '-fvisibility=hidden',
        '-fvisibility-inlines-hidden'
      ]),
      language: 'cpp'
    )
  endif
endif

include_dirs = include_directories('.', 'include')

headers = files('include/snitch/snitch.hpp',
                'include/snitch/snitch_append.hpp',
                'include/snitch/snitch_capture.hpp',
                'include/snitch/snitch_cli.hpp',
                'include/snitch/snitch_concepts.hpp',
                'include/snitch/snitch_console.hpp',
                'include/snitch/snitch_error_handling.hpp',
                'include/snitch/snitch_expression.hpp',
                'include/snitch/snitch_file.hpp',
                'include/snitch/snitch_fixed_point.hpp',
                'include/snitch/snitch_function.hpp',
                'include/snitch/snitch_macros_check.hpp',
                'include/snitch/snitch_macros_check_base.hpp',
                'include/snitch/snitch_macros_consteval.hpp',
                'include/snitch/snitch_macros_constexpr.hpp',
                'include/snitch/snitch_macros_exceptions.hpp',
                'include/snitch/snitch_macros_misc.hpp',
                'include/snitch/snitch_macros_reporter.hpp',
                'include/snitch/snitch_macros_test_case.hpp',
                'include/snitch/snitch_macros_utility.hpp',
                'include/snitch/snitch_macros_warnings.hpp',
                'include/snitch/snitch_matcher.hpp',
                'include/snitch/snitch_registry.hpp',
                'include/snitch/snitch_reporter_catch2_xml.hpp',
                'include/snitch/snitch_reporter_console.hpp',
                'include/snitch/snitch_reporter_teamcity.hpp',
                'include/snitch/snitch_section.hpp',
                'include/snitch/snitch_string.hpp',
                'include/snitch/snitch_string_utility.hpp',
                'include/snitch/snitch_test_data.hpp',
                'include/snitch/snitch_type_name.hpp',
                'include/snitch/snitch_vector.hpp')

sources = files('src/snitch_append.cpp',
               'src/snitch_capture.cpp',
               'src/snitch_cli.cpp',
               'src/snitch_console.cpp',
               'src/snitch_error_handling.cpp',
               'src/snitch_file.cpp',
               'src/snitch_main.cpp',
               'src/snitch_matcher.cpp',
               'src/snitch_registry.cpp',
               'src/snitch_reporter_catch2_xml.cpp',
               'src/snitch_reporter_console.cpp',
               'src/snitch_reporter_teamcity.cpp',
               'src/snitch_section.cpp',
               'src/snitch_string_utility.cpp',
               'src/snitch_test_data.cpp')

if get_option('unity_build')

  main = files('src/snitch.cpp')

else

  main = sources

endif

make_snitch_all = files('make_snitch_all.py')

subdir('snitch')

install_headers(headers, subdir: 'snitch')

if get_option('create_library')

  snitch = library('snitch',
    conf_file, main, headers,
    include_directories: include_dirs,
    install: true,
  )

  snitch_dep = declare_dependency(
    link_with: snitch,
    include_directories: include_dirs
  )

  import('pkgconfig').generate(
    snitch,
    filebase: 'snitch',
    description: 'Lightweight C++20 testing framework.',
    url: 'https://github.com/cschreib/snitch',
  )

else

  snitch_dep = declare_dependency(include_directories: include_dirs)

endif

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('snitch', snitch_dep)
endif

test('snitch_all_test',
  executable('snitch_all_test',
             [snitch_all, 'tests/testing.cpp'],
             cpp_args: ['-DSNITCH_TEST_WITH_SNITCH', '-DSNITCH_TEST_HEADER_ONLY'],
             dependencies: [snitch_dep]
  )
)
