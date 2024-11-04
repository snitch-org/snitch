#ifndef SNITCH_MACROS_TEST_CASE_HPP
#define SNITCH_MACROS_TEST_CASE_HPP

#include "snitch/snitch_config.hpp"
#include "snitch/snitch_macros_utility.hpp"
#include "snitch/snitch_registry.hpp"

#define SNITCH_TEST_CASE_IMPL(ID, ...)                                                             \
    static void        ID();                                                                       \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add({__VA_ARGS__}, SNITCH_CURRENT_LOCATION, &ID);                            \
    void ID()

#if !(SNITCH_DISABLE)
#    define SNITCH_TEST_CASE(...)                                                                  \
        SNITCH_TEST_CASE_IMPL(SNITCH_MACRO_CONCAT(test_fun_, __COUNTER__), __VA_ARGS__)
#else // SNITCH_DISABLE
#    define SNITCH_TEST_CASE(...)                                                                  \
        [[maybe_unused]] static void SNITCH_MACRO_CONCAT(test_id_, __COUNTER__)()
#endif // SNITCH_DISABLE

#define SNITCH_TEMPLATE_LIST_TEST_CASE_IMPL(ID, NAME, TAGS, TYPES)                                 \
    template<typename TestType>                                                                    \
    static void        ID();                                                                       \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_with_type_list<TYPES>(                                                   \
            {NAME, TAGS}, SNITCH_CURRENT_LOCATION, []<typename TestType>() { ID<TestType>(); });   \
    template<typename TestType>                                                                    \
    void ID()

#if !(SNITCH_DISABLE)
#    define SNITCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)                                      \
        SNITCH_TEMPLATE_LIST_TEST_CASE_IMPL(                                                       \
            SNITCH_MACRO_CONCAT(test_fun_, __COUNTER__), NAME, TAGS, TYPES)
#else // SNITCH_DISABLE
#    define SNITCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) SNITCH_VOID_STATEMENT
#endif // SNITCH_DISABLE

#define SNITCH_TEMPLATE_TEST_CASE_IMPL(ID, NAME, TAGS, ...)                                        \
    template<typename TestType>                                                                    \
    static void        ID();                                                                       \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_with_types<__VA_ARGS__>(                                                 \
            {NAME, TAGS}, SNITCH_CURRENT_LOCATION, []<typename TestType>() { ID<TestType>(); });   \
    template<typename TestType>                                                                    \
    void ID()

#if !(SNITCH_DISABLE)
#    define SNITCH_TEMPLATE_TEST_CASE(NAME, TAGS, ...)                                             \
        SNITCH_TEMPLATE_TEST_CASE_IMPL(                                                            \
            SNITCH_MACRO_CONCAT(test_fun_, __COUNTER__), NAME, TAGS, __VA_ARGS__)
#else // SNITCH_DISABLE
#    define SNITCH_TEMPLATE_TEST_CASE(NAME, TAGS, ...)                                             \
        template<typename TestType>                                                                \
        [[maybe_unused]] static void SNITCH_MACRO_CONCAT(test_id_, __COUNTER__)()
#endif // SNITCH_DISABLE

#if !(SNITCH_DISABLE)
#    define SNITCH_TEST_CASE_METHOD_IMPL(ID, FIXTURE, ...)                                         \
        namespace {                                                                                \
        struct ID : FIXTURE {                                                                      \
            void test_fun();                                                                       \
        };                                                                                         \
        }                                                                                          \
        static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =           \
            snitch::tests.add_fixture(                                                             \
                {#FIXTURE, __VA_ARGS__}, SNITCH_CURRENT_LOCATION, []() { ID{}.test_fun(); });      \
        void ID::test_fun()

#    define SNITCH_TEST_CASE_METHOD(FIXTURE, ...)                                                  \
        SNITCH_TEST_CASE_METHOD_IMPL(                                                              \
            SNITCH_MACRO_CONCAT(test_fixture_, __COUNTER__), FIXTURE, __VA_ARGS__)
#else // SNITCH_DISABLE
#    define SNITCH_TEST_CASE_METHOD_IMPL(ID, FIXTURE, ...)                                         \
        namespace {                                                                                \
        struct ID : FIXTURE {                                                                      \
            void test_fun();                                                                       \
        };                                                                                         \
        }                                                                                          \
        inline void ID::test_fun()

#    define SNITCH_TEST_CASE_METHOD(FIXTURE, ...)                                                  \
        SNITCH_TEST_CASE_METHOD_IMPL(SNITCH_MACRO_CONCAT(test_id_, __COUNTER__), FIXTURE)
#endif // SNITCH_DISABLE

#define SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD_IMPL(ID, FIXTURE, NAME, TAGS, TYPES)                 \
    namespace {                                                                                    \
    template<typename TestType>                                                                    \
    struct ID : FIXTURE<TestType> {                                                                \
        void test_fun();                                                                           \
    };                                                                                             \
    }                                                                                              \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_fixture_with_type_list<TYPES>(                                           \
            {#FIXTURE, NAME, TAGS}, SNITCH_CURRENT_LOCATION,                                       \
            []() < typename TestType > { ID<TestType>{}.test_fun(); });                            \
    template<typename TestType>                                                                    \
    void ID<TestType>::test_fun()

#if !(SNITCH_DISABLE)
#    define SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES)                      \
        SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD_IMPL(                                                \
            SNITCH_MACRO_CONCAT(test_fixture_, __COUNTER__), FIXTURE, NAME, TAGS, TYPES)
#else // SNITCH_DISABLE
#    define SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES) SNITCH_VOID_STATEMENT
#endif // SNITCH_DISABLE

#define SNITCH_TEMPLATE_TEST_CASE_METHOD_IMPL(ID, FIXTURE, NAME, TAGS, ...)                        \
    namespace {                                                                                    \
    template<typename TestType>                                                                    \
    struct ID : FIXTURE<TestType> {                                                                \
        void test_fun();                                                                           \
    };                                                                                             \
    }                                                                                              \
    static const char* SNITCH_MACRO_CONCAT(test_id_, __COUNTER__) [[maybe_unused]] =               \
        snitch::tests.add_fixture_with_types<__VA_ARGS__>(                                         \
            {#FIXTURE, NAME, TAGS}, SNITCH_CURRENT_LOCATION,                                       \
            []() < typename TestType > { ID<TestType>{}.test_fun(); });                            \
    template<typename TestType>                                                                    \
    void ID<TestType>::test_fun()

#if !(SNITCH_DISABLE)
#    define SNITCH_TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, ...)                             \
        SNITCH_TEMPLATE_TEST_CASE_METHOD_IMPL(                                                     \
            SNITCH_MACRO_CONCAT(test_fixture_, __COUNTER__), FIXTURE, NAME, TAGS, __VA_ARGS__)
#else // SNITCH_DISABLE
#    define SNITCH_TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, ...) SNITCH_VOID_STATEMENT
#endif // SNITCH_DISABLE

// clang-format off
#if SNITCH_WITH_SHORTHAND_MACROS
#    define TEST_CASE(NAME, ...)                       SNITCH_TEST_CASE(NAME, __VA_ARGS__)
#    define TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) SNITCH_TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES)
#    define TEMPLATE_TEST_CASE(NAME, TAGS, ...)        SNITCH_TEMPLATE_TEST_CASE(NAME, TAGS, __VA_ARGS__)

#    define TEST_CASE_METHOD(FIXTURE, NAME, ...)                       SNITCH_TEST_CASE_METHOD(FIXTURE, NAME, __VA_ARGS__)
#    define TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES) SNITCH_TEMPLATE_LIST_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, TYPES)
#    define TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, ...)        SNITCH_TEMPLATE_TEST_CASE_METHOD(FIXTURE, NAME, TAGS, __VA_ARGS__)
#endif
// clang-format on

#endif
