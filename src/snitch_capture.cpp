#include "snitch/snitch_capture.hpp"

#include "snitch/snitch_console.hpp"
#include "snitch/snitch_registry.hpp"

namespace snitch::impl {
namespace {
void trim(std::string_view& str, std::string_view patterns) noexcept {
    std::size_t start = str.find_first_not_of(patterns);
    if (start == str.npos) {
        return;
    }

    str.remove_prefix(start);

    std::size_t end = str.find_last_not_of(patterns);
    if (end != str.npos) {
        str.remove_suffix(str.size() - end - 1);
    }
}
} // namespace

scoped_capture::~scoped_capture() {
#if SNITCH_WITH_EXCEPTIONS
    if (std::uncaught_exceptions() > 0 && !state.held_info.has_value()) {
        // We are unwinding the stack because an exception has been thrown;
        // keep a copy of the full capture state since we will want to preserve the information
        // when reporting the exception.
        state.held_info = state.info;
    }
#endif

    state.info.captures.resize(state.info.captures.size() - count);
}

std::string_view extract_next_name(std::string_view& names) noexcept {
    std::string_view result;

    auto pos = names.find_first_of(",()\"\"''");

    bool in_string = false;
    bool in_char   = false;
    int  parens    = 0;
    while (pos != names.npos) {
        switch (names[pos]) {
        case '"':
            if (!in_char) {
                in_string = !in_string;
            }
            break;
        case '\'':
            if (!in_string) {
                in_char = !in_char;
            }
            break;
        case '(':
            if (!in_string && !in_char) {
                ++parens;
            }
            break;
        case ')':
            if (!in_string && !in_char) {
                --parens;
            }
            break;
        case ',':
            if (!in_string && !in_char && parens == 0) {
                result = names.substr(0, pos);
                trim(result, " \t\n\r");
                names.remove_prefix(pos + 1);
                return result;
            }
            break;
        }

        pos = names.find_first_of(",()\"\"''", pos + 1);
    }

    std::swap(result, names);
    trim(result, " \t\n\r");
    return result;
}

small_string<max_capture_length>& add_capture(test_state& state) {
    if (state.info.captures.available() == 0) {
        state.reg.print(
            make_colored("error:", state.reg.with_color, color::fail),
            " max number of captures reached; "
            "please increase 'SNITCH_MAX_CAPTURES' (currently ",
            max_captures, ")\n.");
        assertion_failed("max number of captures reached");
    }

#if SNITCH_WITH_EXCEPTIONS
    if (std::uncaught_exceptions() == 0) {
        notify_exception_handled();
    }
#endif

    state.info.captures.grow(1);
    state.info.captures.back().clear();
    return state.info.captures.back();
}
} // namespace snitch::impl
