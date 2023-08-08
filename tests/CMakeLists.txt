include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571 # v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
add_executable(c-capnproto-testcases addressbook.capnp.c capn-stream-test.cpp capn-test.cpp example-test.cpp)
target_link_libraries(c-capnproto-testcases PRIVATE CapnC_Runtime GTest::gtest)

gtest_add_tests(TARGET c-capnproto-testcases)
