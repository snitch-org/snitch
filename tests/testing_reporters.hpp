#include <filesystem>
#include <fstream>
#include <regex>

void register_tests_for_reporters(snitch::registry& r);

const std::filesystem::path test_data_path = std::filesystem::path("data");

struct print_to_file {
    std::ofstream file;

    print_to_file(snitch::registry& r, std::string_view filename) :
        file(test_data_path / "actual" / filename) {
        r.print_callback = {*this, snitch::constant<&print_to_file::print>{}};
    }

    void print(std::string_view msg) noexcept {
        file.write(msg.data(), msg.size());
    }
};

void regex_blank(std::string& line, const std::regex& ignores);
void regex_blank(std::string& line, const std::vector<std::regex>& ignores);

#define CHECK_FOR_DIFFERENCES(ARGS, IGNORES, FILENAME)                                             \
    do {                                                                                           \
        {                                                                                          \
            print_to_file file_override{framework.registry, FILENAME};                             \
            auto          input =                                                                  \
                snitch::cli::parse_arguments(static_cast<int>((ARGS).size()), (ARGS).data());      \
            framework.registry.configure(input.value());                                           \
            framework.registry.run_tests(input.value());                                           \
        }                                                                                          \
        {                                                                                          \
            std::ifstream file_actual(test_data_path / "actual" / (FILENAME));                     \
            std::ofstream file_blanked(test_data_path / "blanked" / (FILENAME));                   \
            std::string   line_actual;                                                             \
            while (std::getline(file_actual, line_actual)) {                                       \
                regex_blank(line_actual, (IGNORES));                                               \
                file_blanked.write(line_actual.data(), line_actual.size());                        \
                file_blanked.write("\n", 1);                                                       \
            }                                                                                      \
        }                                                                                          \
        {                                                                                          \
            INFO("checking ", FILENAME);                                                           \
            std::ifstream file_expected(test_data_path / "expected" / (FILENAME));                 \
            std::ifstream file_actual(test_data_path / "blanked" / (FILENAME));                    \
            std::string   line_expected;                                                           \
            std::string   line_actual;                                                             \
            std::size_t   line_number = 1;                                                         \
            while ((std::getline(file_expected, line_expected),                                    \
                    std::getline(file_actual, line_actual)) &&                                     \
                   file_expected && file_actual) {                                                 \
                CAPTURE(line_number);                                                              \
                CHECK(std::string_view{line_actual} == std::string_view{line_expected});           \
                ++line_number;                                                                     \
            }                                                                                      \
            if (file_expected.is_open() && !file_expected.eof()) {                                 \
                FAIL_CHECK("expected more output");                                                \
            } else if (!file_actual.eof()) {                                                       \
                FAIL_CHECK("unexpected extra output");                                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)
