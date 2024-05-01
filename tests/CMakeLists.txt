include(GoogleTest)

find_package(GTest CONFIG REQUIRED)

add_executable(test-formula formula-test.cpp)
target_link_libraries(test-formula PUBLIC formula GTest::gtest_main)
target_folder(test-formula "Tests")

gtest_discover_tests(test-formula)
