#ifndef SNITCH_FILE_HPP
#define SNITCH_FILE_HPP

#include "snitch/snitch_config.hpp"

#include <string_view>

namespace snitch::impl {
// Maximum length of a file path.
constexpr std::size_t max_path_length = SNITCH_MAX_PATH_LENGTH;

class file_writer {
    void* file_handle = nullptr;

public:
    SNITCH_EXPORT constexpr file_writer() noexcept = default;

    // Requires: permission to write to the given path, path length less than max_path_length
    SNITCH_EXPORT explicit file_writer(std::string_view path);

    file_writer(const file_writer&)            = delete;
    file_writer& operator=(const file_writer&) = delete;

    SNITCH_EXPORT file_writer(file_writer&& other) noexcept;

    SNITCH_EXPORT file_writer& operator=(file_writer&& other) noexcept;

    SNITCH_EXPORT ~file_writer();

    SNITCH_EXPORT void write(std::string_view message) noexcept;
};
} // namespace snitch::impl

#endif
