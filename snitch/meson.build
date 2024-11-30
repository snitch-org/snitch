version = meson.project_version()
vers = version.split('.')

git_command = run_command(find_program('git'), 'log', '-1', '--format=%h', check: false)
if git_command.returncode() == 0
  git_hash = git_command.stdout().strip()
  full_version = version + '.' + git_hash
else
  full_version = version
endif

is_shared = get_option('default_library') == 'shared'

conf_data = configuration_data({
  'PROJECT_VERSION'       : version,
  'PROJECT_VERSION_MAJOR' : vers[0],
  'PROJECT_VERSION_MINOR' : vers[1],
  'PROJECT_VERSION_PATCH' : vers[2],

  'SNITCH_FULL_VERSION' : full_version,

  'SNITCH_MAX_TEST_CASES'           : get_option('max_test_cases'),
  'SNITCH_MAX_NESTED_SECTIONS'      : get_option('max_nested_sections'),
  'SNITCH_MAX_EXPR_LENGTH'          : get_option('max_expr_length'),
  'SNITCH_MAX_MESSAGE_LENGTH'       : get_option('max_message_length'),
  'SNITCH_MAX_TEST_NAME_LENGTH'     : get_option('max_test_name_length'),
  'SNITCH_MAX_TAG_LENGTH'           : get_option('max_tag_length'),
  'SNITCH_MAX_CAPTURES'             : get_option('max_captures'),
  'SNITCH_MAX_CAPTURE_LENGTH'       : get_option('max_capture_length'),
  'SNITCH_MAX_UNIQUE_TAGS'          : get_option('max_unique_tags'),
  'SNITCH_MAX_COMMAND_LINE_ARGS'    : get_option('max_command_line_args'),
  'SNITCH_MAX_REGISTERED_REPORTERS' : get_option('max_registered_reporters'),
  'SNITCH_MAX_PATH_LENGTH'          : get_option('max_path_length'),
  'SNITCH_MAX_REPORTER_SIZE_BYTES'  : get_option('max_reporter_size_bytes'),

  'SNITCH_ENABLE'                          : get_option('enable').to_int(),
  'SNITCH_DEFINE_MAIN'                     : get_option('define_main').to_int(),
  'SNITCH_WITH_EXCEPTIONS'                 : get_option('with_exceptions').to_int(),
  'SNITCH_WITH_MULTITHREADING'             : get_option('with_multithreading').to_int(),
  'SNITCH_WITH_TIMINGS'                    : get_option('with_timings').to_int(),
  'SNITCH_WITH_SHORTHAND_MACROS'           : get_option('with_shorthand_macros').to_int(),
  'SNITCH_CONSTEXPR_FLOAT_USE_BITCAST'     : get_option('constexpr_float_use_bitcast').to_int(),
  'SNITCH_APPEND_TO_CHARS'                 : get_option('snitch_append_to_chars').to_int(),
  'SNITCH_DEFAULT_WITH_COLOR'              : get_option('default_with_color').to_int(),
  'SNITCH_DECOMPOSE_SUCCESSFUL_ASSERTIONS' : get_option('decompose_successful_assertions').to_int(),
  'SNITCH_WITH_ALL_REPORTERS'              : get_option('with_all_reporters').to_int(),
  'SNITCH_WITH_TEAMCITY_REPORTER'          : get_option('with_teamcity_reporter').to_int(),
  'SNITCH_WITH_CATCH2_XML_REPORTER'        : get_option('with_catch2_xml_reporter').to_int(),

  'SNITCH_SHARED_LIBRARY' : is_shared.to_int()
})

conf_file = configure_file(
  input: meson.project_source_root() / 'include/snitch/snitch_config.hpp.config',
  output: 'snitch_config.hpp',
  format: 'cmake',
  configuration: conf_data,
  install_dir: 'include/snitch'
)

snitch_all = custom_target(
  output: 'snitch_all.hpp',
  build_by_default: get_option('create_header_only'),
  depend_files: [conf_file, headers, main, sources],
  command: [find_program('python3'), make_snitch_all,
            meson.project_source_root(), meson.project_build_root()],
  depfile: 'snitch_all.d',
  install: true,
  install_dir: 'include/snitch'
)
