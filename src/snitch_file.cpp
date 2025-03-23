#include "snitch/snitch_file.hpp"

#include "snitch/snitch_append.hpp"
#include "snitch/snitch_error_handling.hpp"

#if SNITCH_WITH_STD_FILE_IO
#    include <cstdio> // for std::fopen, std::fwrite, std::fclose
#else
#    include <exception> // for std::terminate
#endif

namespace snitch::impl {
#if SNITCH_WITH_STD_FILE_IO
class stdio_file {
public:
    std::FILE* handle = nullptr;

    explicit stdio_file(std::string_view path) {}

    ~stdio_file() {
        std::fclose(handle);
    }
};

void stdio_file_open(file_object_storage& storage, std::string_view path) {
    // Unfortunately, fopen() needs a null-terminated string, so need a copy...
    small_string<max_path_length + 1> null_terminated_path;
    if (!append(null_terminated_path, path)) {
        assertion_failed("output file path is too long");
    }

    std::FILE* handle = nullptr;
#    if defined(_MSC_VER)
    // MSVC thinks std::fopen is unsafe.
    fopen_s(&handle, null_terminated_path.data(), "w");
#    else
    handle = std::fopen(null_terminated_path.data(), "w");
#    endif

    if (handle == nullptr) {
        assertion_failed("output file could not be opened for writing");
    }

    storage.emplace<std::FILE*>(handle);
}

void stdio_file_write(const file_object_storage& storage, std::string_view message) noexcept {
    auto handle = storage.get_mutable<std::FILE*>();
    std::fwrite(message.data(), sizeof(char), message.length(), handle);
    std::fflush(handle);
}

void stdio_file_close(file_object_storage& storage) noexcept {
    auto handle = storage.get_mutable<std::FILE*>();
    std::fclose(handle);
    storage.reset();
}
#else
// No default file I/O; it is expected that the user will use
// their own implementation wherever a file is needed.
void stdio_file_open(file_object_storage&, std::string_view) {
    std::terminate();
}

void stdio_file_write(const file_object_storage&, std::string_view) noexcept {
    std::terminate();
}

void stdio_file_close(file_object_storage& storage) noexcept {
    std::terminate();
}
#endif

file_writer::file_writer(std::string_view path) {
    snitch::io::file_open(storage, path);
}

file_writer::file_writer(file_writer&& other) noexcept {
    storage = std::move(other.storage);
}

file_writer& file_writer::operator=(file_writer&& other) noexcept {
    close();
    storage = std::move(other.storage);
    return *this;
}

file_writer::~file_writer() {
    close();
}

void file_writer::write(std::string_view message) noexcept {
    if (!storage.has_value()) {
        return;
    }

    snitch::io::file_write(storage, message);
}

bool file_writer::is_open() noexcept {
    return storage.has_value();
}

void file_writer::close() noexcept {
    if (!storage.has_value()) {
        return;
    }

    snitch::io::file_close(storage);
}
} // namespace snitch::impl

namespace snitch::io {
function_ref<void(file_object_storage& storage, std::string_view path)> file_open =
    &impl::stdio_file_open;

function_ref<void(const file_object_storage& storage, std::string_view message) noexcept>
    file_write = &impl::stdio_file_write;

function_ref<void(file_object_storage& storage) noexcept> file_close = &impl::stdio_file_close;
} // namespace snitch::io
