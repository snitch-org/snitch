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
    // Requires: permission to write to the given path, path length less than max_path_length
    explicit file_writer(std::string_view path);

    file_writer(const file_writer&)            = delete;
    file_writer& operator=(const file_writer&) = delete;

    file_writer(file_writer&& other) noexcept;
    file_writer& operator=(file_writer&& other) noexcept;

    ~file_writer();

    void write(std::string_view message) noexcept;
};
} // namespace snitch::impl

#endif
