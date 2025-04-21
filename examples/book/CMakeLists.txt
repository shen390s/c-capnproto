cmake_minimum_required(VERSION 3.10)
project(MyCapnpProject)

# Find the Cap'n Proto compiler and C plugin
find_program(CAPNP_EXECUTABLE capnp)

if(NOT CAPNP_EXECUTABLE)
    message(FATAL_ERROR "capnp compiler not found!")
endif()

# Schema file (adjust path as needed)
set(SCHEMA_FILE "book.capnp")

# Generated file paths (outputs in the build directory)
set(GENERATED_C "${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_FILE}.c")
set(GENERATED_H "${CMAKE_CURRENT_BINARY_DIR}/${SCHEMA_FILE}.h")

# Generate C++ sources from the schema
add_custom_command(
    OUTPUT ${GENERATED_C} ${GENERATED_H}
    COMMAND ${CAPNP_EXECUTABLE} compile
        -o $<TARGET_FILE:capnpc-c>:${CMAKE_CURRENT_BINARY_DIR}
	-I ${CMAKE_CURRENT_SOURCE_DIR}/../../compiler
	--src-prefix ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_FILE}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SCHEMA_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating C code for ${SCHEMA_FILE}"
)

# Create an executable/library and include generated files
add_executable(book test.c ${GENERATED_C})

# Include directory for generated headers
target_include_directories(book PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link against Cap'n Proto libraries
target_link_libraries(book PRIVATE
    capnp
    CapnC_Runtime
)
