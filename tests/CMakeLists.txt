include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)

set(PROCMINE_TEST_SOURCES
    algorithm_test.cpp
    database_test.cpp
    log_test.cpp
)

add_executable(procmine_tests ${PROCMINE_TEST_SOURCES})
target_link_libraries(procmine_tests
    PRIVATE
        procmine
        gtest
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(procmine_tests) 