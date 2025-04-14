#ifndef SNITCH_FILE_HPP
#define SNITCH_FILE_HPP

#include "snitch/snitch_any.hpp"
#include "snitch/snitch_config.hpp"

#include <string_view>

namespace snitch {
// Maximum length of a file path.
constexpr std::size_t max_path_length = SNITCH_MAX_PATH_LENGTH;
// Maximum size of a file object, in bytes.
constexpr std::size_t max_file_object_size_bytes = SNITCH_MAX_FILE_OBJECT_SIZE_BYTES;

using file_object_storage = inplace_any<max_file_object_size_bytes>;
} // namespace snitch

namespace snitch::impl {
SNITCH_EXPORT void stdio_file_open(file_object_storage& storage, std::string_view path);

SNITCH_EXPORT void
stdio_file_write(const file_object_storage& storage, std::string_view message) noexcept;

SNITCH_EXPORT void stdio_file_close(file_object_storage& storage) noexcept;

class file_writer {
    file_object_storage storage;

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

    SNITCH_EXPORT bool is_open() noexcept;

    SNITCH_EXPORT void close() noexcept;
};
} // namespace snitch::impl

namespace snitch::io {
// Requires: permission to write to the given path, path length less than max_path_length
SNITCH_EXPORT extern function_ref<void(file_object_storage& storage, std::string_view path)>
    file_open;

SNITCH_EXPORT extern function_ref<void(
    const file_object_storage& storage, std::string_view message) noexcept>
    file_write;

SNITCH_EXPORT extern function_ref<void(file_object_storage& storage) noexcept> file_close;
} // namespace snitch::io

#endif
