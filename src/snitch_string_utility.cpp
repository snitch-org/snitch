#include "snitch/snitch_string_utility.hpp"

#include <algorithm> // for std::rotate
#include <cstring> // for std::memcpy

namespace snitch {
bool replace_all(
    small_string_span string, std::string_view pattern, std::string_view replacement) noexcept {

    if (replacement.size() == pattern.size()) {
        std::string_view sv(string.begin(), string.size());
        auto             pos = sv.find(pattern);

        while (pos != sv.npos) {
            // Replace pattern by replacement
            std::memcpy(string.data() + pos, replacement.data(), replacement.size());
            pos += replacement.size();

            // Find next occurrence
            pos = sv.find(pattern, pos);
        }

        return true;
    } else if (replacement.size() < pattern.size()) {
        const std::size_t char_diff = pattern.size() - replacement.size();
        std::string_view  sv(string.begin(), string.size());
        auto              pos = sv.find(pattern);

        while (pos != sv.npos) {
            // Shift data after the replacement to the left to fill the gap
            std::rotate(string.begin() + pos, string.begin() + pos + char_diff, string.end());
            string.resize(string.size() - char_diff);

            // Replace pattern by replacement
            std::memcpy(string.data() + pos, replacement.data(), replacement.size());
            pos += replacement.size();

            // Find next occurrence
            sv  = {string.begin(), string.size()};
            pos = sv.find(pattern, pos);
        }

        return true;
    } else {
        const std::size_t char_diff = replacement.size() - pattern.size();
        std::string_view  sv(string.begin(), string.size());
        auto              pos      = sv.find(pattern);
        bool              overflow = false;

        while (pos != sv.npos) {
            // Shift data after the pattern to the right to make room for the replacement
            const std::size_t char_growth = std::min(char_diff, string.available());
            if (char_growth != char_diff) {
                overflow = true;
            }
            string.grow(char_growth);

            if (char_diff <= string.size() && string.size() - char_diff > pos) {
                std::rotate(string.begin() + pos, string.end() - char_diff, string.end());
            }

            // Replace pattern by replacement
            const std::size_t max_chars = std::min(replacement.size(), string.size() - pos);
            std::memcpy(string.data() + pos, replacement.data(), max_chars);
            pos += max_chars;

            // Find next occurrence
            sv  = {string.begin(), string.size()};
            pos = sv.find(pattern, pos);
        }

        return !overflow;
    }
}

bool is_match(std::string_view string, std::string_view regex) noexcept {
    // An empty regex matches any string; early exit.
    // An empty string matches an empty regex (exit here) or any regex containing
    // only wildcards (exit later).
    if (regex.empty()) {
        return true;
    }

    const std::size_t regex_size  = regex.size();
    const std::size_t string_size = string.size();

    // Iterate characters of the regex string and exit at first non-match.
    std::size_t js = 0;
    for (std::size_t jr = 0; jr < regex_size; ++jr, ++js) {
        bool escaped = false;
        if (regex[jr] == '\\') {
            // Escaped character, look ahead ignoring special characters.
            ++jr;
            if (jr >= regex_size) {
                // Nothing left to escape; the regex is ill-formed.
                return false;
            }

            escaped = true;
        }

        if (!escaped && regex[jr] == '*') {
            // Wildcard is found; if this is the last character of the regex
            // then any further content will be a match; early exit.
            if (jr == regex_size - 1) {
                return true;
            }

            // Discard what has already been matched.
            regex = regex.substr(jr + 1);

            // If there are no more characters in the string after discarding, then we only match if
            // the regex contains only wildcards from there on.
            const std::size_t remaining = string_size >= js ? string_size - js : 0u;
            if (remaining == 0u) {
                return regex.find_first_not_of('*') == regex.npos;
            }

            // Otherwise, we loop over all remaining characters of the string and look
            // for a match when starting from each of them.
            for (std::size_t o = 0; o < remaining; ++o) {
                if (is_match(string.substr(js + o), regex)) {
                    return true;
                }
            }

            return false;
        } else if (js >= string_size || regex[jr] != string[js]) {
            // Regular character is found; not a match if not an exact match in the string.
            return false;
        }
    }

    // We have finished reading the regex string and did not find either a definite non-match
    // or a definite match. This means we did not have any wildcard left, hence that we need
    // an exact match. Therefore, only match if the string size is the same as the regex.
    return js == string_size;
}
} // namespace snitch
