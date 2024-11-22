#include "snitch/snitch_main.hpp"

#include "snitch/snitch_cli.hpp"
#include "snitch/snitch_registry.hpp"

namespace snitch {
SNITCH_EXPORT int main(int argc, char* argv[]) {
    if constexpr (snitch::is_enabled) {
        std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
        if (!args) {
            return 1;
        }
        snitch::tests.configure(*args);
        return snitch::tests.run_tests(*args) ? 0 : 1;
    } else {
        return 0;
    }
}
} // namespace snitch

#if SNITCH_DEFINE_MAIN
SNITCH_EXPORT int main(int argc, char* argv[]) {
    return snitch::main(argc, argv);
}
#endif
