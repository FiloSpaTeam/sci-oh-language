cmake_minimum_required(VERSION 3.16)

project(scioh VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(sci-oh
    src/main.cpp
    src/scioh/Codegen.cpp
    src/scioh/Lexer.cpp
    src/scioh/Parser.cpp
)

target_include_directories(sci-oh PRIVATE src)

enable_testing()

add_test(
    NAME smoke_emit_cpp
    COMMAND sci-oh --emit-cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/ciao.sci
)
set_tests_properties(smoke_emit_cpp PROPERTIES
    PASS_REGULAR_EXPRESSION "int main"
)

add_test(
    NAME smoke_condition_emit_cpp
    COMMAND sci-oh --emit-cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/ciao.sci
)
set_tests_properties(smoke_condition_emit_cpp PROPERTIES
    PASS_REGULAR_EXPRESSION "Scioh::isTruthy"
)

add_test(
    NAME smoke_logic_emit_cpp
    COMMAND sci-oh --emit-cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/ciao.sci
)
set_tests_properties(smoke_logic_emit_cpp PROPERTIES
    PASS_REGULAR_EXPRESSION "Scioh::logicAnd"
)

add_test(
    NAME smoke_function_emit_cpp
    COMMAND sci-oh --emit-cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/funzionale.sci
)
set_tests_properties(smoke_function_emit_cpp PROPERTIES
    PASS_REGULAR_EXPRESSION "Scioh::apply"
)

add_test(
    NAME smoke_return_emit_cpp
    COMMAND sci-oh --emit-cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/funzionale.sci
)
set_tests_properties(smoke_return_emit_cpp PROPERTIES
    PASS_REGULAR_EXPRESSION "ReturnValue"
)

add_test(
    NAME smoke_firstclass_emit_cpp
    COMMAND sci-oh --emit-cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/funzionale.sci
)
set_tests_properties(smoke_firstclass_emit_cpp PROPERTIES
    PASS_REGULAR_EXPRESSION "std::function"
)

add_test(
    NAME version_prints_motto
    COMMAND sci-oh --version
)
set_tests_properties(version_prints_motto PROPERTIES
    PASS_REGULAR_EXPRESSION "Lu compilatore che compila quann ie pare"
)
