struct event_deep_copy {
    enum class type {
        unknown,
        test_run_started,
        test_run_ended,
        test_case_started,
        test_case_ended,
        test_case_skipped,
        assertion_failed,
        assertion_succeeded
    };

    type event_type = type::unknown;

    snitch::small_string<snitch::max_test_name_length> test_run_name;

    bool        test_run_success                         = false;
    std::size_t test_run_run_count                       = 0;
    std::size_t test_run_fail_count                      = 0;
    std::size_t test_run_allowed_fail_count              = 0;
    std::size_t test_run_skip_count                      = 0;
    std::size_t test_run_assertion_count                 = 0;
    std::size_t test_run_assertion_failure_count         = 0;
    std::size_t test_run_allowed_assertion_failure_count = 0;

    snitch::test_case_state test_case_state = snitch::test_case_state::success;

    std::size_t test_case_assertion_count        = 0;
    std::size_t test_case_failure_count          = 0;
    std::size_t test_case_expected_failure_count = 0;

    snitch::small_string<snitch::max_test_name_length> test_id_name;
    snitch::small_string<snitch::max_test_name_length> test_id_tags;
    snitch::small_string<snitch::max_test_name_length> test_id_type;

    snitch::small_string<snitch::max_message_length> location_file;
    std::size_t                                      location_line = 0u;

    snitch::small_string<snitch::max_message_length> expr_message;
    snitch::small_string<64>                         expr_type;
    snitch::small_string<snitch::max_message_length> expr_expected;
    snitch::small_string<snitch::max_message_length> expr_actual;
    snitch::small_vector<snitch::small_string<snitch::max_message_length>, snitch::max_captures>
        captures;
    snitch::
        small_vector<snitch::small_string<snitch::max_message_length>, snitch::max_nested_sections>
            sections;
};

event_deep_copy deep_copy(const snitch::event::data& e);

struct mock_framework {
    snitch::registry registry;

    snitch::impl::test_case test_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snitch::impl::test_case_state::not_run};

    snitch::small_vector<event_deep_copy, 32> events;
    snitch::small_string<4086>                messages;
    bool                                      catch_success = false;

    mock_framework() noexcept;

    void report(const snitch::registry&, const snitch::event::data& e) noexcept;
    void print(std::string_view msg) noexcept;

    void setup_reporter();
    void setup_print();
    void setup_reporter_and_print();

    void run_test();

    std::optional<event_deep_copy> get_failure_event(std::size_t id = 0) const;
    std::optional<event_deep_copy> get_success_event(std::size_t id = 0) const;
    std::optional<event_deep_copy> get_skip_event() const;

    std::size_t get_num_registered_tests() const;
    std::size_t get_num_runs() const;
    std::size_t get_num_failures() const;
    std::size_t get_num_successes() const;
    std::size_t get_num_skips() const;
};

struct console_output_catcher {
    snitch::small_string<4086>                              messages = {};
    snitch::small_function<void(std::string_view) noexcept> prev_print;

    console_output_catcher() : prev_print(snitch::cli::console_print) {
        snitch::cli::console_print = {*this, snitch::constant<&console_output_catcher::print>{}};
    }

    ~console_output_catcher() {
        snitch::cli::console_print = prev_print;
    }

    void print(std::string_view msg) noexcept {
        append_or_truncate(messages, msg);
    }
};

using arg_vector = snitch::small_vector<const char*, snitch::max_command_line_args>;

struct cli_input {
    std::string_view scenario;
    arg_vector       args;
};

template<std::size_t MaxEvents>
struct event_catcher {
    snitch::registry mock_registry;

    snitch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snitch::impl::test_case_state::not_run};

    snitch::impl::test_state mock_test{.reg = mock_registry, .test = mock_case};

    snitch::small_vector<event_deep_copy, MaxEvents> events;

    event_catcher() {
        mock_registry.report_callback = {*this, snitch::constant<&event_catcher::report>{}};
        mock_registry.verbose         = snitch::registry::verbosity::full;
    }

    void report(const snitch::registry&, const snitch::event::data& e) noexcept {
        events.push_back(deep_copy(e));
    }
};

struct test_override {
    snitch::impl::test_state* previous;

    template<std::size_t N>
    explicit test_override(event_catcher<N>& catcher) :
        previous(snitch::impl::try_get_current_test()) {
        snitch::impl::set_current_test(&catcher.mock_test);
    }

    ~test_override() {
        snitch::impl::set_current_test(previous);
    }
};

namespace snitch::matchers {
struct has_expr_data {
    struct expr_data {
        std::string_view type;
        std::string_view expected;
        std::string_view actual;
    };

    std::variant<std::string_view, expr_data> expected;

    explicit has_expr_data(std::string_view msg);
    explicit has_expr_data(
        std::string_view type, std::string_view expected, std::string_view actual);

    bool match(const event_deep_copy& e) const noexcept;

    small_string<max_message_length>
    describe_match(const event_deep_copy& e, match_status status) const noexcept;
};
} // namespace snitch::matchers

#define CHECK_EVENT_TEST_ID(ACTUAL, EXPECTED)                                                      \
    CHECK(ACTUAL.test_id_name == EXPECTED.name);                                                   \
    CHECK(ACTUAL.test_id_tags == EXPECTED.tags);                                                   \
    CHECK(ACTUAL.test_id_type == EXPECTED.type)

#define CHECK_EVENT_LOCATION(ACTUAL, FILE, LINE)                                                   \
    CHECK(ACTUAL.location_file == std::string_view(FILE));                                         \
    CHECK(ACTUAL.location_line == LINE)

#define CHECK_CAPTURES_FOR_FAILURE(FAILURE_ID, ...)                                                \
    do {                                                                                           \
        auto failure = framework.get_failure_event(FAILURE_ID);                                    \
        REQUIRE(failure.has_value());                                                              \
        const char* EXPECTED_CAPTURES[] = {__VA_ARGS__};                                           \
        REQUIRE(                                                                                   \
            failure.value().captures.size() == sizeof(EXPECTED_CAPTURES) / sizeof(const char*));   \
        std::size_t CAPTURE_INDEX = 0;                                                             \
        for (std::string_view CAPTURED_VALUE : EXPECTED_CAPTURES) {                                \
            CHECK(failure.value().captures[CAPTURE_INDEX] == CAPTURED_VALUE);                      \
            ++CAPTURE_INDEX;                                                                       \
        }                                                                                          \
    } while (0)

#define CHECK_CAPTURES(...) CHECK_CAPTURES_FOR_FAILURE(0u, __VA_ARGS__)

#define CHECK_NO_CAPTURE_FOR_FAILURE(FAILURE_ID)                                                   \
    do {                                                                                           \
        auto failure = framework.get_failure_event(FAILURE_ID);                                    \
        REQUIRE(failure.has_value());                                                              \
        CHECK(failure.value().captures.empty());                                                   \
    } while (0)

#define CHECK_NO_CAPTURE CHECK_NO_CAPTURE_FOR_FAILURE(0u)

#define CHECK_SECTIONS_FOR_FAILURE(FAILURE_ID, ...)                                                \
    do {                                                                                           \
        auto failure = framework.get_failure_event(FAILURE_ID);                                    \
        REQUIRE(failure.has_value());                                                              \
        const char* EXPECTED_SECTIONS[] = {__VA_ARGS__};                                           \
        REQUIRE(                                                                                   \
            failure.value().sections.size() == sizeof(EXPECTED_SECTIONS) / sizeof(const char*));   \
        std::size_t SECTION_INDEX = 0;                                                             \
        for (std::string_view SECTION_NAME : EXPECTED_SECTIONS) {                                  \
            CHECK(failure.value().sections[SECTION_INDEX] == SECTION_NAME);                        \
            ++SECTION_INDEX;                                                                       \
        }                                                                                          \
    } while (0)

#define CHECK_SECTIONS(...) CHECK_SECTIONS_FOR_FAILURE(0u, __VA_ARGS__)

#define CHECK_NO_SECTION_FOR_FAILURE(FAILURE_ID)                                                   \
    do {                                                                                           \
        auto failure = framework.get_failure_event(FAILURE_ID);                                    \
        REQUIRE(failure.has_value());                                                              \
        CHECK(failure.value().sections.empty());                                                   \
    } while (0)

#define CHECK_NO_SECTION CHECK_NO_SECTION_FOR_FAILURE(0u)

#define CHECK_RUN(                                                                                 \
    SUCCESS, RUN_COUNT, FAIL_COUNT, EXP_FAIL_COUNT, SKIP_COUNT, ASSERT_COUNT, FAILURE_COUNT,       \
    EXP_FAILURE_COUNT)                                                                             \
    do {                                                                                           \
        REQUIRE(framework.events.size() >= 2u);                                                    \
        auto end = framework.events.back();                                                        \
        REQUIRE(end.event_type == event_deep_copy::type::test_run_ended);                          \
        CHECK(end.test_run_success == SUCCESS);                                                    \
        CHECK(end.test_run_run_count == RUN_COUNT);                                                \
        CHECK(end.test_run_fail_count == FAIL_COUNT);                                              \
        CHECK(end.test_run_allowed_fail_count == EXP_FAIL_COUNT);                                  \
        CHECK(end.test_run_skip_count == SKIP_COUNT);                                              \
        CHECK(end.test_run_assertion_count == ASSERT_COUNT);                                       \
        CHECK(end.test_run_assertion_failure_count == FAILURE_COUNT);                              \
        CHECK(end.test_run_allowed_assertion_failure_count == EXP_FAILURE_COUNT);                  \
    } while (0)

#define CHECK_CASE(STATE, ASSERT_COUNT, FAILURE_COUNT)                                             \
    do {                                                                                           \
        REQUIRE(framework.events.size() >= 2u);                                                    \
        auto end = framework.events.back();                                                        \
        REQUIRE(end.event_type == event_deep_copy::type::test_case_ended);                         \
        CHECK(end.test_case_state == STATE);                                                       \
        CHECK(end.test_case_assertion_count == ASSERT_COUNT);                                      \
        CHECK(end.test_case_failure_count == FAILURE_COUNT);                                       \
        CHECK(end.test_case_expected_failure_count == 0u);                                         \
    } while (0)

#define CHECK_EVENT(CATCHER, EVENT, TYPE, FAILURE_LINE, ...)                                       \
    do {                                                                                           \
        CHECK((EVENT).event_type == (TYPE));                                                       \
        CHECK_EVENT_TEST_ID((EVENT), (CATCHER).mock_case.id);                                      \
        CHECK_EVENT_LOCATION((EVENT), __FILE__, (FAILURE_LINE));                                   \
        CHECK((EVENT) == snitch::matchers::has_expr_data{__VA_ARGS__});                            \
    } while (0)

#define CHECK_EXPR(CATCHER, EVENT_TYPE, FAILURE_LINE, ...)                                         \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 1u);                                                  \
        REQUIRE((CATCHER).events.size() == 1u);                                                    \
        CHECK_EVENT(CATCHER, (CATCHER).events[0], EVENT_TYPE, FAILURE_LINE, __VA_ARGS__);          \
    } while (0)

#define CHECK_EVENT_FAILURE(CATCHER, EVENT, FAILURE_LINE, ...)                                     \
    CHECK_EVENT(CATCHER, EVENT, event_deep_copy::type::assertion_failed, FAILURE_LINE, __VA_ARGS__)

#define CHECK_EXPR_FAILURE(CATCHER, FAILURE_LINE, ...)                                             \
    CHECK_EXPR(CATCHER, event_deep_copy::type::assertion_failed, FAILURE_LINE, __VA_ARGS__)

#define CHECK_EXPR_SUCCESS(CATCHER)                                                                \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 1u);                                                  \
        REQUIRE((CATCHER).events.size() == 1u);                                                    \
        CHECK((CATCHER).events[0].event_type == event_deep_copy::type::assertion_succeeded);       \
        CHECK_EVENT_TEST_ID((CATCHER).events[0], (CATCHER).mock_case.id);                          \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_SUCCESS(CATCHER)                                                      \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
        CHECK((CATCHER).events[0].event_type == event_deep_copy::type::assertion_succeeded);       \
        CHECK((CATCHER).events[1].event_type == event_deep_copy::type::assertion_succeeded);       \
        CHECK_EVENT_TEST_ID((CATCHER).events[0], (CATCHER).mock_case.id);                          \
        CHECK_EVENT_TEST_ID((CATCHER).events[1], (CATCHER).mock_case.id);                          \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_FAILURE(CATCHER)                                                      \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
        CHECK(                                                                                     \
            (((CATCHER).events[0].event_type == event_deep_copy::type::assertion_succeeded) ^      \
             ((CATCHER).events[1].event_type == event_deep_copy::type::assertion_succeeded)));     \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_FAILURE_2(CATCHER)                                                    \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
        CHECK((CATCHER).events[0].event_type == event_deep_copy::type::assertion_failed);          \
        CHECK((CATCHER).events[1].event_type == event_deep_copy::type::assertion_failed);          \
    } while (0)
