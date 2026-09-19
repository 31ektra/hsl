cmake_minimum_required(VERSION 3.20)

project(
    HSL
    VERSION 9.0.0
    DESCRIPTION "The native programming language of Helios"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HSL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(HSL_ENABLE_HARDENING "Enable supported compiler hardening" ON)

option(HSL_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
if(HSL_ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_executable(hsl
    src/main.cpp
    src/ast.cpp
    src/code_generator.cpp
    src/compiler.cpp
    src/diagnostic.cpp
    src/lexer.cpp
    src/module_loader.cpp
    src/parser.cpp
    src/semantic.cpp
    src/source.cpp
    src/token.cpp
    src/type.cpp
    src/type_checker.cpp
)

target_include_directories(hsl PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(hsl PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(HSL_WARNINGS_AS_ERRORS)
    target_compile_options(hsl PRIVATE -Werror)
endif()

if(HSL_ENABLE_HARDENING AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hsl PRIVATE
        -fstack-protector-strong
        -fno-strict-overflow
    )
    if(UNIX AND NOT APPLE)
        target_link_options(hsl PRIVATE -Wl,-z,relro,-z,now)
    endif()
endif()

install(TARGETS hsl RUNTIME DESTINATION bin)

include(CTest)


enable_testing()

add_test(
    NAME lexer
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/lexer/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)


add_test(
    NAME parser
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/parser/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)


add_test(
    NAME semantic
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/semantic/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)


add_test(
    NAME type_checker
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/type_checker/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)


add_test(
    NAME code_generation
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/code_generation/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)


add_test(
    NAME modules
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/modules/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)


add_test(
    NAME compiler
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/compiler/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(NAME adversarial COMMAND bash ${PROJECT_SOURCE_DIR}/tests/adversarial/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})

add_test(
    NAME self_hosted_foundation
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/self_hosted/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME bootstrap_validation
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/bootstrap/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME enums
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/enums/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME match
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/match/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME targets
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/targets/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME system_api
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/system_api/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME collections
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/collections/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME arena
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/arena/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME ownership_flow
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/ownership_flow/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME release_metadata
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/release/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME runtime_3
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/runtime_3/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME ownership_match
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/ownership_match/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME release_3
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/release_3/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(NAME runtime_4 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/runtime_4/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})
add_test(NAME cli_4 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/cli_4/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})
add_test(NAME release_4 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/release_4/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})

add_test(NAME runtime_5 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/runtime_5/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})
add_test(NAME release_5 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/release_5/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})

add_test(NAME toolchain_6 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/toolchain_6/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})
add_test(NAME release_6 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/release_6/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})

add_test(NAME bootstrap_7 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/bootstrap_7/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})
add_test(NAME release_7 COMMAND bash ${PROJECT_SOURCE_DIR}/tests/release_7/run_tests.sh $<TARGET_FILE:hsl> ${PROJECT_SOURCE_DIR})

add_test(
    NAME bootstrap_8
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/bootstrap_8/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME release_8
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/release_8/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME kernel_types
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/kernel_types/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)

add_test(
    NAME release_9
    COMMAND bash
        ${PROJECT_SOURCE_DIR}/tests/release_9/run_tests.sh
        $<TARGET_FILE:hsl>
        ${PROJECT_SOURCE_DIR}
)
