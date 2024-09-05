#include <snitch/snitch.hpp>

int main(int argc, char* argv[]) {
    // Parse the command-line arguments.
    std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
    if (!args) {
        return 1;
    }

    snitch::tests.configure(*args);
    return snitch::tests.run_tests(*args) ? 0 : 1;
}
