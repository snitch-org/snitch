project('snitch', 'cpp',
  default_options : ['cpp_std=c++20', 'default_library=static'],
  version : '1.0.0'
)

subdir('snitch')

headers = files(
  'include/snitch/snitch.hpp',
  'include/snitch/snitch_teamcity.hpp'
)

install_headers(headers, subdir: 'snitch')

if get_option('create_library')

  pkg = import('pkgconfig')

  snitch = library('snitch',
    conf_file, 'src/snitch.cpp', 'include/snitch/snitch_teamcity.hpp',
    include_directories: 'include',
    install: true,
    cpp_args: snitch_config_cpp_args,
  )

  snitch_dep = declare_dependency(
    link_with: snitch,
    include_directories : include_directories('.','include')
  )

  pkg.generate(
    snitch,
    filebase: 'snitch',
    description: 'Lightweight C++20 testing framework.',
    url: 'https://github.com/cschreib/snitch',
  )

else

  snitch_dep = declare_dependency(
    include_directories : include_directories('.','include')
  )

endif

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('snitch', snitch_dep)
endif

python = import('python').find_installation('python3')

snitch_single_header = custom_target(
  output: 'snitch_all.hpp',
  build_by_default: get_option('create_header_only'),
  depend_files : [conf_file, headers, 'src/snitch.cpp'],
  command: [python, files('make_snitch_all.py'),
            meson.project_source_root(), meson.project_build_root()],
  depfile: 'snitch_single_header.d',
  install_dir: 'include/snitch',
)

snitch_single_header_dep = declare_dependency(
  include_directories : include_directories('.')
)
