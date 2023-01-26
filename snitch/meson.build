version = meson.project_version()
vers = version.split('.')

conf_data = configuration_data()

conf_data.set('PROJECT_VERSION', version)
conf_data.set('PROJECT_VERSION_MAJOR', vers[0])
conf_data.set('PROJECT_VERSION_MINOR', vers[1])
conf_data.set('PROJECT_VERSION_PATCH', vers[2])

git_hash = run_command(find_program('git'),'log','-1','--format=%h'
                      ,check:true).stdout().strip()
conf_data.set('SNITCH_FULL_VERSION', version + '.' + git_hash)

conf_data.set('SNITCH_MAX_TEST_CASES'         ,get_option('max_test_cases'),        description : 'Maximum number of test cases in a test application.')
conf_data.set('SNITCH_MAX_NESTED_SECTIONS'    ,get_option('max_nested_sections'),   description : 'Maximum depth of nested sections in a test case.')
conf_data.set('SNITCH_MAX_EXPR_LENGTH'        ,get_option('max_expr_length'),       description : 'Maximum length of a printed expression when reporting failure.')
conf_data.set('SNITCH_MAX_MESSAGE_LENGTH'     ,get_option('max_message_length'),    description : 'Maximum length of error or status messages.')
conf_data.set('SNITCH_MAX_TEST_NAME_LENGTH'   ,get_option('max_test_name_length'),  description : 'Maximum length of a test case name.')
conf_data.set('SNITCH_MAX_TAG_LENGTH'         ,get_option('max_tag_length'),        description : 'Maximum length of a test tag.')
conf_data.set('SNITCH_MAX_CAPTURES'           ,get_option('max_captures'),          description : 'Maximum number of captured expressions in a test case.')
conf_data.set('SNITCH_MAX_CAPTURE_LENGTH'     ,get_option('max_capture_length'),    description : 'Maximum length of a captured expression.')
conf_data.set('SNITCH_MAX_UNIQUE_TAGS'        ,get_option('max_unique_tags'),       description : 'Maximum number of unique tags in a test application.')
conf_data.set('SNITCH_MAX_COMMAND_LINE_ARGS'  ,get_option('max_command_line_args'), description : 'Maximum number of command line arguments to a test application.')

conf_data.set10('SNITCH_DEFINE_MAIN'          ,get_option('define_main'),           description : 'Define main() in snitch -- disable to provide your own main() function.')
conf_data.set10('SNITCH_WITH_EXCEPTIONS'      ,get_option('with_exceptions'),       description : 'Use exceptions in snitch implementation -- will be forced OFF if exceptions are not available.')
conf_data.set10('SNITCH_WITH_TIMINGS'         ,get_option('with_timings'),          description : 'Measure the time taken by each test case -- disable to speed up tests.')
conf_data.set10('SNITCH_WITH_SHORTHAND_MACROS',get_option('with_shorthand_macros'), description : 'Use short names for test macros -- disable if this causes conflicts.')
conf_data.set10('SNITCH_DEFAULT_WITH_COLOR'   ,get_option('default_with_color'),    description : 'Enable terminal colors by default -- can also be controlled by command line interface.')
conf_data.set10('SNITCH_CREATE_HEADER_ONLY'   ,get_option('create_header_only'),    description : 'Create a single-header header-only version of snitch.')
conf_data.set10('SNITCH_CREATE_LIBRARY'       ,get_option('create_library'),        description : 'Build a compiled library version of snitch.')

conf_file = configure_file(
  input: meson.project_source_root() + '/include/snitch/snitch_config.hpp.config',
  output: 'snitch_config.hpp',
  format: 'cmake',
  configuration: conf_data,
  install_dir: 'include/snitch'
)

snitch_config_cpp_args = [
  '-DSNITCH_MAX_TEST_CASES='        + conf_data.get('SNITCH_MAX_TEST_CASES').to_string(),
  '-DSNITCH_MAX_NESTED_SECTIONS='   + conf_data.get('SNITCH_MAX_NESTED_SECTIONS').to_string(),
  '-DSNITCH_MAX_EXPR_LENGTH='       + conf_data.get('SNITCH_MAX_EXPR_LENGTH').to_string(),
  '-DSNITCH_MAX_MESSAGE_LENGTH='    + conf_data.get('SNITCH_MAX_MESSAGE_LENGTH').to_string(),
  '-DSNITCH_MAX_TEST_NAME_LENGTH='  + conf_data.get('SNITCH_MAX_TEST_NAME_LENGTH').to_string(),
  '-DSNITCH_MAX_TAG_LENGTH='        + conf_data.get('SNITCH_MAX_TAG_LENGTH').to_string(),

  '-DSNITCH_MAX_UNIQUE_TAGS='       + conf_data.get('SNITCH_MAX_UNIQUE_TAGS').to_string(),
  '-DSNITCH_MAX_COMMAND_LINE_ARGS=' + conf_data.get('SNITCH_MAX_COMMAND_LINE_ARGS').to_string(),

  '-DSNITCH_DEFINE_MAIN='           + conf_data.get('SNITCH_DEFINE_MAIN').to_string(),
  '-DSNITCH_WITH_EXCEPTIONS='       + conf_data.get('SNITCH_WITH_EXCEPTIONS').to_string(),
  '-DSNITCH_WITH_TIMINGS='          + conf_data.get('SNITCH_WITH_TIMINGS').to_string(),
  '-DSNITCH_WITH_SHORTHAND_MACROS=' + conf_data.get('SNITCH_WITH_SHORTHAND_MACROS').to_string(),
  '-DSNITCH_DEFAULT_WITH_COLOR='    + conf_data.get('SNITCH_DEFAULT_WITH_COLOR').to_string(),
]
