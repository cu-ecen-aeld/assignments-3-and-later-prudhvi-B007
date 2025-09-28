cmake_minimum_required(VERSION 3.0.0)
project(aesd-assignments)

set(CMAKE_C_FLAGS "-pthread")

set(AUTOTEST_SOURCES
    test/assignment1/Test_hello.c
    test/assignment1/Test_assignment_validate.c
    test/assignment4/Test_threading.c
)

set(TESTED_SOURCE
    ../examples/autotest-validate/autotest-validate.c
    ../examples/threading/threading.c
)

add_subdirectory(assignment-autotest)

