cmake_minimum_required(VERSION 3.22)
if(BUILD_TESTING OR NOT DEFINED BUILD_TESTING)
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

# Boilerplate DK CMAKE PROJ 2023-09-25.1
set(default_build_shared)
if(NOT (ANDROID OR IOS))
    set(default_build_shared ON)
endif()
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ${default_build_shared})

# Build hygiene: Tools for keeping the source code clean. By default
# we try to enforce build hygiene.
# You can override with either BUILD_HYGIENE=DISABLED or
# BUILD_HYGIENE=ENABLED.
# Hygiene is only done if the project is top-level.
if(PROJECT_IS_TOP_LEVEL AND NOT BUILD_HYGIENE STREQUAL DISABLED)
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

# add_runtime_library(
#       NAME_SUFFIX <suffix>
#       [EXCLUDE_FROM_ALL]
#       [MODE STATIC|STATIC_EXPORTS|SHARED]
# )
#
# Without a MODE the library is built according to CMake conventions (BUILD_SHARED_LIBS).
#
# NAME_SUFFIX <suffix>
#       A suffix added to the CMake names.
#
#       The target will be named `CapnC_Runtime<SUFFIX>`.
#       The alias will be named `CapnC::Runtime<SUFFIX>`.
#
# EXCLUDE_FROM_ALL
#
#       Do not add the library to the ALL target.
#
# MODE STATIC|STATIC_EXPORTS|SHARED
#
#       STATIC
#       Build a static library without export symbols, regardless of BUILD_SHARED_LIBS.
#
#       STATIC_EXPORTS
#       Build a static library with export symbols, regardless of BUILD_SHARED_LIBS.
#
#       SHARED
#       Build a shared library, regardless of BUILD_SHARED_LIBS.
function(add_runtime_library)
        set(noValues EXCLUDE_FROM_ALL)
        set(singleValues NAME_SUFFIX MODE)
        set(multiValues)
        cmake_parse_arguments(PARSE_ARGV 0 ARG "${noValues}" "${singleValues}" "${multiValues}")

        # Nice names which can be used without shadowing even in add_subdirectory()
        set(C_CAPNPROTO_ALIAS CapnC::Runtime${ARG_NAME_SUFFIX})
        set(C_CAPNPROTO_TARGET CapnC_Runtime${ARG_NAME_SUFFIX})
        set(C_CAPNPROTO_EXPORTS ${BUILD_SHARED_LIBS})
        set(C_CAPNPROTO_LINKAGE)
        set(PROP_EXCLUDE_FROM_ALL)
        if(ARG_EXCLUDE_FROM_ALL)
                set(PROP_EXCLUDE_FROM_ALL EXCLUDE_FROM_ALL)
        endif()
        if(NOT ARG_MODE)
        elseif(ARG_MODE STREQUAL STATIC)
                set(C_CAPNPROTO_EXPORTS OFF)
                set(C_CAPNPROTO_LINKAGE STATIC)
        elseif(ARG_MODE STREQUAL STATIC_EXPORTS)
                set(C_CAPNPROTO_EXPORTS ON)
                set(C_CAPNPROTO_LINKAGE STATIC)
        elseif(ARG_MODE STREQUAL SHARED)
                set(C_CAPNPROTO_EXPORTS ON)
                set(C_CAPNPROTO_LINKAGE SHARED)
        else()
                message(FATAL_ERROR "No [MODE ${ARG_MODE}] is supported")
        endif()

        add_library(${C_CAPNPROTO_TARGET} ${C_CAPNPROTO_LINKAGE} ${PROP_EXCLUDE_FROM_ALL}
                lib/capn.c
                lib/capn-malloc.c
                lib/capn-stream.c
                lib/capnp_c.h)
        add_library(${C_CAPNPROTO_ALIAS} ALIAS ${C_CAPNPROTO_TARGET})
        set_target_properties(${C_CAPNPROTO_TARGET} PROPERTIES
                EXPORT_NAME Runtime${ARG_NAME_SUFFIX}
                POSITION_INDEPENDENT_CODE ON
                WINDOWS_EXPORT_ALL_SYMBOLS ON)
        target_include_directories(${C_CAPNPROTO_TARGET}
                PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/compiler>
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib>
                $<INSTALL_INTERFACE:include>)
        if(C_CAPNPROTO_EXPORTS)
                set_target_properties(${C_CAPNPROTO_TARGET} PROPERTIES
                    WINDOWS_EXPORT_ALL_SYMBOLS ON)
        endif()
endfunction()

add_runtime_library()
add_runtime_library(NAME_SUFFIX -Shared             EXCLUDE_FROM_ALL MODE SHARED)
add_runtime_library(NAME_SUFFIX -Static             EXCLUDE_FROM_ALL MODE STATIC)
add_runtime_library(NAME_SUFFIX -StaticExports      EXCLUDE_FROM_ALL MODE STATIC_EXPORTS)

add_executable(capnpc-c
        compiler/capnpc-c.c
        compiler/schema.capnp.c
        compiler/str.c)
target_link_libraries(capnpc-c CapnC::Runtime)
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

option(C_CAPNPROTO_ENABLE_INSTALL "Add instructions for 'cmake --install' of CapnC_Runtime library and capnpc-c executable" ON)
if(C_CAPNPROTO_ENABLE_INSTALL)
    install(TARGETS CapnC_Runtime capnpc-c
            EXPORT CapnC)
    install(EXPORT CapnC
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CapnC
            NAMESPACE CapnC::
            FILE CapnCConfig.cmake)
    install(FILES lib/capnp_c.h TYPE INCLUDE)
endif()

if(BUILD_TESTING AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(tests)
endif()
