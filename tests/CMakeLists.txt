# googletest
# ----------
#
# The 1.14.x branch requires at least C++14.
#
# In particular, 19.34+ (MSVC 2022) is required for Visual Studio:
# Avoid GoogleTest 1.14.0 error:
#   [build] C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include\yvals_core.h(811):
#    error STL1001: Unexpected compiler version, expected MSVC 19.34 or newer.
#   [build] C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include\yvals_core.h(811):
#    error C2338: Error in C++ Standard Library usage.
#   [build] C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.35.32215\include\type_traits(1767):
#    error C2275: '_Ty1': illegal use of this type as an expression
set(GIT_TAG)
if(MSVC)
  if(MSVC_VERSION LESS 1934)
    message(STATUS "Using GoogleTest 1.13 because MSVC 19.34+ (MSVC 2022) is needed for GoogleTest 1.14+")
    set(GIT_TAG b796f7d44681514f58a683a3a71ff17c94edb0c1) # v1.13.0
  endif()
elseif(NOT "cxx_std_14" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  message(STATUS "Using GoogleTest 1.13 because C++14 is needed for GoogleTest 1.14+")
  set(GIT_TAG b796f7d44681514f58a683a3a71ff17c94edb0c1) # v1.13.0
endif()
if(NOT GIT_TAG)
  set(GIT_TAG f8d7d77c06936315286eb55f8de22cd23c188571) # v1.14.0
endif()

include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG ${GIT_TAG}
)
FetchContent_MakeAvailable(googletest)

add_executable(c-capnproto-testcases addressbook.capnp.c capn-stream-test.cpp capn-test.cpp example-test.cpp)
target_link_libraries(c-capnproto-testcases PRIVATE CapnC_Runtime GTest::gtest)

include(GoogleTest)
gtest_add_tests(TARGET c-capnproto-testcases TEST_LIST testCases)

if(WIN32)
  # On Windows gtest.dll must be in PATH or in current directory.
  foreach(testCase IN LISTS testCases)
    set_property(TEST ${testCase} APPEND PROPERTY
      ENVIRONMENT_MODIFICATION
      PATH=path_list_prepend:${PROJECT_BINARY_DIR}/bin)
  endforeach()
endif()
