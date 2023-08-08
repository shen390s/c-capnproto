# Run locally the -ubsan Sanitizer with:
#   rm -rf build/
#   ./dk dksdk.cmake.link
#   .ci/cmake/bin/cmake --preset=ci-linux_x86_64-sanitizers-ubsan
#   .ci/cmake/bin/ctest -S ci/ctest/Sanitizers-UBSAN-CTest.cmake --extra-verbose

# Determine project directories
if(NOT OUR_PROJECT_SOURCE_DIR)
    set(OUR_PROJECT_SOURCE_DIR "${CTEST_SCRIPT_DIRECTORY}/../..")
    cmake_path(NORMAL_PATH OUR_PROJECT_SOURCE_DIR)
endif()
if(NOT OUR_PROJECT_BINARY_DIR)
    cmake_path(APPEND OUR_PROJECT_SOURCE_DIR build OUTPUT_VARIABLE OUR_PROJECT_BINARY_DIR)
endif()

# Re-use test details
include(${OUR_PROJECT_SOURCE_DIR}/CTestConfig.cmake)

# Read the CI generated CMakeCache.txt
load_cache("${OUR_PROJECT_BINARY_DIR}"
        READ_WITH_PREFIX CACHED_
        CMAKE_BUILD_TYPE)

# Basic information every run should set
site_name(CTEST_SITE)
set(CTEST_BUILD_NAME ${CMAKE_HOST_SYSTEM_NAME})
set(CTEST_SOURCE_DIRECTORY "${OUR_PROJECT_SOURCE_DIR}")
set(CTEST_BINARY_DIRECTORY "${OUR_PROJECT_BINARY_DIR}")
set(CTEST_CMAKE_GENERATOR Ninja)
if(CACHED_CMAKE_BUILD_TYPE)
    set(CTEST_CONFIGURATION_TYPE ${CACHED_CMAKE_BUILD_TYPE})
else()
    set(CTEST_CONFIGURATION_TYPE Debug)
endif()

#   Tell CTest to add to UBSAN_OPTIONS to capture its logs
set(CTEST_MEMORYCHECK_TYPE UndefinedBehaviorSanitizer)

# UBSAN_OPTIONS
# --------
#
#   Even if we have [UBSAN_OPTIONS] defined in CMakePresets.json,
#   that will not impact this script which is invoked by `ctest` not
#   `cmake`, and [CTEST_MEMORYCHECK_TYPE] will overwrite it anyway.
#
#   Enable stack traces
set(extra_OPTIONS print_stacktrace=1)
#   Apply to ctest_memcheck()
set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS ${extra_OPTIONS})
#   Apply to ctest_build()
if(extra_OPTIONS)
    if(ENV{UBSAN_OPTIONS})
        set(ENV{UBSAN_OPTIONS} ${extra_OPTIONS}:$ENV{UBSAN_OPTIONS})
    else()
        set(ENV{UBSAN_OPTIONS} ${extra_OPTIONS})
    endif()
endif()

ctest_start(Experimental)

ctest_build(RETURN_VALUE err)
if(err)
    message(FATAL_ERROR "CTest build error ${err}")
endif()

ctest_memcheck(RETURN_VALUE err)
if(err)
    message(FATAL_ERROR "CTest memcheck error ${err}")
endif()
