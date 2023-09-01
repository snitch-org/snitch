#include "snitch/snitch_file.hpp"

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_error_handling.hpp"

#include <cstdio> // for std::fwrite

namespace snitch::impl {
file_writer::file_writer(std::string_view path) {
    // Unfortunately, fopen() needs a null-terminated string, so need a copy...
    small_string<max_path_length + 1> null_terminated_path;
    if (!append(null_terminated_path, path)) {
        assertion_failed("output file path is too long");
    }

#if defined(_MSC_VER)
    // MSVC thinks std::fopen is unsafe.
    std::FILE* tmp_handle = nullptr;
    fopen_s(&tmp_handle, null_terminated_path.data(), "w");
    file_handle = tmp_handle;
#else
    file_handle = std::fopen(null_terminated_path.data(), "w");
#endif

    if (file_handle == nullptr) {
        assertion_failed("output file could not be opened for writing");
    }
}

file_writer::file_writer(file_writer&& other) noexcept {
    std::swap(file_handle, other.file_handle);
}

file_writer& file_writer::operator=(file_writer&& other) noexcept {
    std::swap(file_handle, other.file_handle);
    return *this;
}

file_writer::~file_writer() {
    if (file_handle == nullptr) {
        return;
    }

    std::fclose(static_cast<std::FILE*>(file_handle));
}

void file_writer::write(std::string_view message) noexcept {
    if (file_handle == nullptr) {
        return;
    }

    std::fwrite(
        message.data(), sizeof(char), message.length(), static_cast<std::FILE*>(file_handle));
    std::fflush(static_cast<std::FILE*>(file_handle));
}
} // namespace snitch::impl
