project('snitch', 'cpp',
  default_options : ['cpp_std=c++20', 'default_library=static'],
  version : '1.0.0'
)

include_dirs = include_directories('.','include')

headers = files('include/snitch/snitch.hpp',
                'include/snitch/snitch_teamcity.hpp')

main = files('src/snitch.cpp')

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
    include_directories : include_dirs
  )

  import('pkgconfig').generate(
    snitch,
    filebase: 'snitch',
    description: 'Lightweight C++20 testing framework.',
    url: 'https://github.com/cschreib/snitch',
  )

else

  snitch_dep = declare_dependency(include_directories : include_dirs)

endif

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('snitch', snitch_dep)
endif
