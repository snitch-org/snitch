#if SNITCH_WITH_EXCEPTIONS
struct assertion_exception : public std::exception {
    snitch::small_string<snitch::max_message_length> message = {};

    explicit assertion_exception(std::string_view msg) {
        snitch::append_or_truncate(message, msg);
        if (message.available() > 0) {
            message.push_back('\0');
        } else {
            message.back() = '\0';
        }
    }

    const char* what() const noexcept override {
        return message.data();
    }
};

struct assertion_exception_enabler {
    snitch::function_ref<void(std::string_view)> prev_handler;

    assertion_exception_enabler() : prev_handler(snitch::assertion_failed_handler) {
        snitch::assertion_failed_handler = [](std::string_view msg) {
            throw assertion_exception(msg);
        };
    }

    ~assertion_exception_enabler() {
        snitch::assertion_failed_handler = prev_handler;
    }
};
#endif
