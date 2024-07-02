#include <memory>

namespace owning_event {
using filter_info  = snitch::small_vector<std::string_view, 4>;
using section_info = snitch::small_vector<snitch::section, snitch::max_nested_sections>;
using capture_info = snitch::small_vector<std::string_view, snitch::max_captures>;

struct test_run_started {
    std::string_view name    = {};
    filter_info      filters = {};
};

struct test_run_ended {
    std::string_view name    = {};
    filter_info      filters = {};

    std::size_t run_count          = 0;
    std::size_t fail_count         = 0;
    std::size_t allowed_fail_count = 0;
    std::size_t skip_count         = 0;

    std::size_t assertion_count                 = 0;
    std::size_t assertion_failure_count         = 0;
    std::size_t allowed_assertion_failure_count = 0;

#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif

    bool success = true;
};

struct test_case_started {
    snitch::test_id         id       = {};
    snitch::source_location location = {};
};

struct test_case_ended {
    snitch::test_id         id       = {};
    snitch::source_location location = {};

    std::size_t assertion_count                 = 0;
    std::size_t assertion_failure_count         = 0;
    std::size_t allowed_assertion_failure_count = 0;

    snitch::test_case_state state = snitch::test_case_state::success;

#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif

    bool failure_expected = false;
    bool failure_allowed  = false;
};

struct section_started {
    snitch::section_id      id       = {};
    snitch::source_location location = {};
};

struct section_ended {
    snitch::section_id      id       = {};
    snitch::source_location location = {};
    bool            skipped                         = false;
    std::size_t     assertion_count                 = 0;
    std::size_t     assertion_failure_count         = 0;
    std::size_t     allowed_assertion_failure_count = 0;
#if SNITCH_WITH_TIMINGS
    float duration = 0.0f;
#endif
};

struct assertion_failed {
    snitch::test_id         id       = {};
    section_info            sections = {};
    capture_info            captures = {};
    snitch::source_location location = {};
    snitch::assertion_data  data     = {};
    bool                    expected = false;
    bool                    allowed  = false;
};

struct assertion_succeeded {
    snitch::test_id         id       = {};
    section_info            sections = {};
    capture_info            captures = {};
    snitch::source_location location = {};
    snitch::assertion_data  data     = {};
};

struct test_case_skipped {
    snitch::test_id         id       = {};
    section_info            sections = {};
    capture_info            captures = {};
    snitch::source_location location = {};
    std::string_view        message  = {};
};

struct list_test_run_started {
    std::string_view name    = {};
    filter_info      filters = {};
};

struct list_test_run_ended {
    std::string_view name    = {};
    filter_info      filters = {};
};

struct test_case_listed {
    snitch::test_id         id       = {};
    snitch::source_location location = {};
};

using data = std::variant<
    owning_event::test_run_started,
    owning_event::test_run_ended,
    owning_event::test_case_started,
    owning_event::test_case_ended,
    owning_event::section_started,
    owning_event::section_ended,
    owning_event::assertion_failed,
    owning_event::assertion_succeeded,
    owning_event::test_case_skipped,
    owning_event::list_test_run_started,
    owning_event::list_test_run_ended,
    owning_event::test_case_listed>;
} // namespace owning_event

owning_event::data deep_copy(snitch::small_string_span pool, const snitch::event::data& e);

template<snitch::signed_integral IndexType>
std::size_t wrap_index(IndexType sid, std::size_t size) noexcept {
    if (sid >= 0) {
        return static_cast<std::size_t>(sid);
    } else {
        std::size_t mid = static_cast<std::size_t>(-sid);
        if (mid < size) {
            return size - mid;
        } else {
            return size;
        }
    }
}

template<snitch::unsigned_integral IndexType>
std::size_t wrap_index(IndexType sid, std::size_t) noexcept {
    return sid;
}

template<typename T, typename IndexType>
std::optional<T>
get_event(snitch::small_vector_span<const owning_event::data> events, IndexType sid) noexcept {
    std::size_t id = wrap_index(sid, events.size());
    if (id >= events.size()) {
        return {};
    }
    if (const T* e = std::get_if<T>(&events[id])) {
        return *e;
    } else {
        return {};
    }
}

template<typename T>
bool is_event(const owning_event::data& e) noexcept {
    return std::get_if<T>(&e) != nullptr;
}

std::optional<owning_event::assertion_failed>
get_failure_event(snitch::small_vector_span<const owning_event::data> events, std::size_t id = 0);

std::optional<owning_event::assertion_succeeded>
get_success_event(snitch::small_vector_span<const owning_event::data> events, std::size_t id = 0);

std::optional<snitch::test_id>         get_test_id(const owning_event::data& e) noexcept;
std::optional<snitch::source_location> get_location(const owning_event::data& e) noexcept;

struct mock_framework {
    struct large_data {
        snitch::registry                             registry;
        snitch::small_string<4086>                   string_pool;
        snitch::small_vector<owning_event::data, 32> events;
    };

    // Put large data on the heap; this can consume too much stack.
    std::unique_ptr<large_data>                   data        = std::make_unique<large_data>();
    snitch::registry&                             registry    = data->registry;
    snitch::small_string<4086>&                   string_pool = data->string_pool;
    snitch::small_vector<owning_event::data, 32>& events      = data->events;

    snitch::impl::test_case test_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snitch::impl::test_case_state::not_run};

    bool catch_success = false;

    mock_framework() noexcept;

    void report(const snitch::registry&, const snitch::event::data& e) noexcept;
    void print(std::string_view msg) noexcept;

    void setup_reporter();

    void run_test();

    template<typename T, typename IndexType>
    std::optional<T> get_event(IndexType id) const noexcept {
        return ::get_event<T>(events, id);
    }

    template<typename T, typename IndexType>
    bool is_event(IndexType id) const noexcept {
        return get_event<T>(id).has_value();
    }

    std::optional<owning_event::assertion_failed>    get_failure_event(std::size_t id = 0) const;
    std::optional<owning_event::assertion_succeeded> get_success_event(std::size_t id = 0) const;

    std::size_t get_num_registered_tests() const;
    std::size_t get_num_runs() const;
    std::size_t get_num_failures() const;
    std::size_t get_num_successes() const;
    std::size_t get_num_skips() const;
    std::size_t get_num_listed_tests() const;

    bool is_test_listed(const snitch::test_id& id) const;
};

struct console_output_catcher {
    struct large_data {
        snitch::small_string<4086> messages;
    };

    // Put large data on the heap; this can consume too much stack.
    std::unique_ptr<large_data> data     = std::make_unique<large_data>();
    snitch::small_string<4086>& messages = data->messages;

    snitch::function_ref<void(std::string_view) noexcept> prev_print;

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
    struct large_data {
        snitch::registry                                    registry;
        snitch::small_string<1024>                          string_pool;
        snitch::small_vector<owning_event::data, MaxEvents> events;
    };

    // Put large data on the heap; this can consume too much stack.
    std::unique_ptr<large_data>                          data     = std::make_unique<large_data>();
    snitch::registry&                                    registry = data->registry;
    snitch::small_string<1024>&                          string_pool = data->string_pool;
    snitch::small_vector<owning_event::data, MaxEvents>& events      = data->events;

    snitch::impl::test_case mock_case{
        .id    = {"mock_test", "[mock_tag]", "mock_type"},
        .func  = nullptr,
        .state = snitch::impl::test_case_state::not_run};

    snitch::impl::test_state mock_test{.reg = registry, .test = mock_case};

    event_catcher() {
        registry.report_callback = {*this, snitch::constant<&event_catcher::report>{}};
        registry.verbose         = snitch::registry::verbosity::full;
    }

    void run_test() {
        registry.run(mock_case);
    }

    void report(const snitch::registry&, const snitch::event::data& e) noexcept {
        events.push_back(deep_copy(string_pool, e));
    }

    template<typename T, typename IndexType>
    std::optional<T> get_event(IndexType id) const noexcept {
        return ::get_event<T>(events, id);
    }

    template<typename T, typename IndexType>
    bool is_event(IndexType id) const noexcept {
        return get_event<T>(id).has_value();
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

    bool match(const owning_event::data& e) const noexcept;

    small_string<max_message_length>
    describe_match(const owning_event::data& e, match_status status) const noexcept;
};
} // namespace snitch::matchers

#define CHECK_EVENT_TEST_ID(ACTUAL, EXPECTED)                                                      \
    do {                                                                                           \
        if (auto id = get_test_id(ACTUAL); id.has_value()) {                                       \
            CHECK(id->name == EXPECTED.name);                                                      \
            CHECK(id->tags == EXPECTED.tags);                                                      \
            CHECK(id->type == EXPECTED.type);                                                      \
        } else {                                                                                   \
            FAIL_CHECK("event has no test ID");                                                    \
        }                                                                                          \
    } while (0)

#define CHECK_EVENT_LOCATION(ACTUAL, FILE, LINE)                                                   \
    do {                                                                                           \
        if (auto l = get_location(ACTUAL); l.has_value()) {                                        \
            CHECK(l->file == std::string_view(FILE));                                              \
            CHECK(l->line == LINE);                                                                \
        } else {                                                                                   \
            FAIL_CHECK("event has no location");                                                   \
        }                                                                                          \
    } while (0)

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
            CHECK(                                                                                 \
                failure.value().sections[SECTION_INDEX].id.name ==                                 \
                std::string_view{SECTION_NAME});                                                   \
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
        if (auto end = framework.get_event<owning_event::test_run_ended>(-1); end.has_value()) {   \
            CHECK(end->success == SUCCESS);                                                        \
            CHECK(end->run_count == RUN_COUNT);                                                    \
            CHECK(end->fail_count == FAIL_COUNT);                                                  \
            CHECK(end->allowed_fail_count == EXP_FAIL_COUNT);                                      \
            CHECK(end->skip_count == SKIP_COUNT);                                                  \
            CHECK(end->assertion_count == ASSERT_COUNT);                                           \
            CHECK(end->assertion_failure_count == FAILURE_COUNT);                                  \
            CHECK(end->allowed_assertion_failure_count == EXP_FAILURE_COUNT);                      \
        } else {                                                                                   \
            FAIL_CHECK("last event is not test_run_ended");                                        \
        }                                                                                          \
    } while (0)

#define CHECK_CASE(STATE, ASSERT_COUNT, FAILURE_COUNT)                                             \
    do {                                                                                           \
        REQUIRE(framework.events.size() >= 2u);                                                    \
        if (auto end = framework.get_event<owning_event::test_case_ended>(-1); end.has_value()) {  \
            CHECK(end->state == STATE);                                                            \
            CHECK(end->assertion_count == ASSERT_COUNT);                                           \
            CHECK(end->assertion_failure_count == FAILURE_COUNT);                                  \
            CHECK(end->allowed_assertion_failure_count == 0u);                                     \
        } else {                                                                                   \
            FAIL_CHECK("last event is not test_case_ended");                                       \
        }                                                                                          \
    } while (0)

#define CHECK_EVENT(CATCHER, EVENT, TYPE, FAILURE_LINE, ...)                                       \
    do {                                                                                           \
        CHECK(is_event<TYPE>(EVENT));                                                              \
        CHECK_EVENT_TEST_ID((EVENT), (CATCHER).mock_case.id);                                      \
        CHECK_EVENT_LOCATION((EVENT), __FILE__, (FAILURE_LINE));                                   \
        CHECK((EVENT) == snitch::matchers::has_expr_data{__VA_ARGS__});                            \
    } while (0)

#define CHECK_EXPR(CATCHER, EVENT_TYPE, FAILURE_LINE, ...)                                         \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 1u);                                                  \
        REQUIRE((CATCHER).events.size() == 1u);                                                    \
        CHECK_EVENT(CATCHER, (CATCHER).events[0u], EVENT_TYPE, FAILURE_LINE, __VA_ARGS__);         \
    } while (0)

#define CHECK_EVENT_FAILURE(CATCHER, EVENT, FAILURE_LINE, ...)                                     \
    CHECK_EVENT(CATCHER, EVENT, owning_event::assertion_failed, FAILURE_LINE, __VA_ARGS__)

#define CHECK_EXPR_FAILURE(CATCHER, FAILURE_LINE, ...)                                             \
    CHECK_EXPR(CATCHER, owning_event::assertion_failed, FAILURE_LINE, __VA_ARGS__)

#define CHECK_EXPR_SUCCESS(CATCHER)                                                                \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 1u);                                                  \
        REQUIRE((CATCHER).events.size() == 1u);                                                    \
        CHECK((CATCHER).is_event<owning_event::assertion_succeeded>(0u));                          \
        CHECK_EVENT_TEST_ID((CATCHER).events[0u], (CATCHER).mock_case.id);                         \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_SUCCESS(CATCHER)                                                      \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
        CHECK((CATCHER).is_event<owning_event::assertion_succeeded>(0u));                          \
        CHECK((CATCHER).is_event<owning_event::assertion_succeeded>(1u));                          \
        CHECK_EVENT_TEST_ID((CATCHER).events[0u], (CATCHER).mock_case.id);                         \
        CHECK_EVENT_TEST_ID((CATCHER).events[1u], (CATCHER).mock_case.id);                         \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_FAILURE(CATCHER)                                                      \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
        CHECK(                                                                                     \
            ((CATCHER).is_event<owning_event::assertion_failed>(0u) ^                              \
             (CATCHER).is_event<owning_event::assertion_failed>(1u)));                             \
    } while (0)

#define CONSTEXPR_CHECK_EXPR_FAILURE_2(CATCHER)                                                    \
    do {                                                                                           \
        CHECK((CATCHER).mock_test.asserts == 2u);                                                  \
        REQUIRE((CATCHER).events.size() == 2u);                                                    \
        CHECK((CATCHER).is_event<owning_event::assertion_failed>(0u));                             \
        CHECK((CATCHER).is_event<owning_event::assertion_failed>(1u));                             \
    } while (0)
