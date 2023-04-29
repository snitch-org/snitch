#include "snitch/snitch_cli.hpp"
#include "snitch/snitch_registry.hpp"

#if SNITCH_DEFINE_MAIN
int main(int argc, char* argv[]) {
    std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
    if (!args) {
        return 1;
    }

    snitch::tests.configure(*args);

    return snitch::tests.run_tests(*args) ? 0 : 1;
}
#endif
