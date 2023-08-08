cmake_minimum_required(VERSION 3.22)
if(BUILD_TESTING)
    # The tests are C++
    set(languages C CXX)
else()
    set(languages C)
endif()
project(c-capnproto LANGUAGES ${languages})

include(CTest)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS NO)
if(BUILD_TESTING)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_EXTENSIONS NO)
endif()

# Stop noise if CXX compiler is set.
set(ignoreAndAvoidCMakeUnusedWarning "${CMAKE_CXX_COMPILER}")

# Boilerplate DK CMAKE PROJ 2023-05-17.1
if(APPLE)
    set(CMAKE_MACOSX_RPATH OFF)
endif()

# Boilerplate DK CMAKE PROJ 2023-05-17.2
if(BUILD_SHARED_LIBS)
    # SHARED is implied by BUILD_SHARED_LIBS=ON
    set(linkage)
else()
    set(linkage STATIC)
endif()

# Build hygiene: Tools for keeping the source code clean. By default
# we try to enforce build hygiene.
# You can override with either BUILD_HYGIENE=DISABLED or
# BUILD_HYGIENE=ENABLED.
if(NOT BUILD_HYGIENE STREQUAL DISABLED)
    if(BUILD_HYGIENE STREQUAL ENABLED)
        set(find_program_args REQUIRED)
    else()
        set(find_program_args)
    endif()
    find_program(CLANG_TIDY_PROGRAM
            NAMES clang-tidy
            PATHS
            # On macOS the easiest way to get clang-tidy is to install the
            # keg-only (not installed into PATH) big `llvm` package.
            /opt/homebrew/opt/llvm/bin
            /usr/local/opt/llvm/bin
            ${find_program_args})
endif()

add_library(CapnC_Runtime ${linkage}
        lib/capn.c
        lib/capn-malloc.c
        lib/capn-stream.c
        lib/capnp_c.h)
add_library(CapnC::Runtime ALIAS CapnC_Runtime)
set_target_properties(CapnC_Runtime PROPERTIES
        EXPORT_NAME Runtime
        WINDOWS_EXPORT_ALL_SYMBOLS ON)
target_include_directories(CapnC_Runtime
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compiler>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:include>)

add_executable(capnpc-c
        compiler/capnpc-c.c
        compiler/schema.capnp.c
        compiler/str.c)
target_link_libraries(capnpc-c CapnC_Runtime)
target_include_directories(capnpc-c
        PRIVATE lib)

# Boilerplate DK CMAKE PROJ 2023-05-17.3
if(BUILD_SHARED_LIBS)
    if(APPLE)
        set(base @loader_path)
    else()
        set(base $ORIGIN)
    endif()
    file(RELATIVE_PATH relDir
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
    set(CMAKE_INSTALL_RPATH
        # best practice RPATH locations when installed
        ${base}
        ${base}/${relDir})
endif()

install(TARGETS CapnC_Runtime capnpc-c
        EXPORT CapnC)
install(EXPORT CapnC
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CapnC
        NAMESPACE CapnC::
        FILE CapnCConfig.cmake)
install(FILES lib/capnp_c.h TYPE INCLUDE)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
