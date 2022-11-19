namespace {
struct event_deep_copy {
    enum class type { unknown, test_case_started, test_case_ended, assertion_failed };

    type event_type = type::unknown;

    snatch::small_string<snatch::max_test_name_length> test_id_name;
    snatch::small_string<snatch::max_test_name_length> test_id_tags;
    snatch::small_string<snatch::max_test_name_length> test_id_type;

    snatch::small_string<snatch::max_message_length> location_file;
    std::size_t                                      location_line = 0u;

    snatch::small_string<snatch::max_message_length> message;
    snatch::small_vector<snatch::small_string<snatch::max_message_length>, snatch::max_captures>
        captures;
};

event_deep_copy deep_copy(const snatch::event::data& e) {
    return std::visit(
        snatch::overload{
            [](const snatch::event::assertion_failed& a) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::assertion_failed;
                append_or_truncate(c.test_id_name, a.id.name);
                append_or_truncate(c.test_id_tags, a.id.tags);
                append_or_truncate(c.test_id_type, a.id.type);
                append_or_truncate(c.location_file, a.location.file);
                c.location_line = a.location.line;
                append_or_truncate(c.message, a.message);
                for (const auto& ac : a.captures) {
                    c.captures.push_back(ac);
                }
                return c;
            },
            [](const snatch::event::test_case_started& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_started;
                append_or_truncate(c.test_id_name, s.id.name);
                append_or_truncate(c.test_id_tags, s.id.tags);
                append_or_truncate(c.test_id_type, s.id.type);
                return c;
            },
            [](const snatch::event::test_case_ended& s) {
                event_deep_copy c;
                c.event_type = event_deep_copy::type::test_case_ended;
                append_or_truncate(c.test_id_name, s.id.name);
                append_or_truncate(c.test_id_tags, s.id.tags);
                append_or_truncate(c.test_id_type, s.id.type);
                return c;
            },
            [](const auto&) -> event_deep_copy { snatch::terminate_with("event not handled"); }},
        e);
}
} // namespace

#define CHECK_EVENT_TEST_ID(ACTUAL, EXPECTED)                                                      \
    CHECK(ACTUAL.test_id_name == EXPECTED.name);                                                   \
    CHECK(ACTUAL.test_id_tags == EXPECTED.tags);                                                   \
    CHECK(ACTUAL.test_id_type == EXPECTED.type)

#define CHECK_EVENT_LOCATION(ACTUAL, FILE, LINE)                                                   \
    CHECK(ACTUAL.location_file == std::string_view(FILE));                                         \
    CHECK(ACTUAL.location_line == LINE)
