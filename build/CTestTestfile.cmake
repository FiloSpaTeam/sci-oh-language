# CMake generated Testfile for 
# Source directory: /workspace
# Build directory: /workspace/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(smoke_emit_cpp "/workspace/build/sci-oh" "--emit-cpp" "/workspace/examples/ciao.sci")
set_tests_properties(smoke_emit_cpp PROPERTIES  PASS_REGULAR_EXPRESSION "int main" _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;20;add_test;/workspace/CMakeLists.txt;0;")
add_test(smoke_condition_emit_cpp "/workspace/build/sci-oh" "--emit-cpp" "/workspace/examples/ciao.sci")
set_tests_properties(smoke_condition_emit_cpp PROPERTIES  PASS_REGULAR_EXPRESSION "Scioh::isTruthy" _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;28;add_test;/workspace/CMakeLists.txt;0;")
add_test(smoke_logic_emit_cpp "/workspace/build/sci-oh" "--emit-cpp" "/workspace/examples/ciao.sci")
set_tests_properties(smoke_logic_emit_cpp PROPERTIES  PASS_REGULAR_EXPRESSION "Scioh::logicAnd" _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;36;add_test;/workspace/CMakeLists.txt;0;")
add_test(smoke_while_emit_cpp "/workspace/build/sci-oh" "--emit-cpp" "/workspace/examples/ciao.sci")
set_tests_properties(smoke_while_emit_cpp PROPERTIES  PASS_REGULAR_EXPRESSION "while \\(Scioh::isTruthy" _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;44;add_test;/workspace/CMakeLists.txt;0;")
add_test(version_prints_motto "/workspace/build/sci-oh" "--version")
set_tests_properties(version_prints_motto PROPERTIES  PASS_REGULAR_EXPRESSION "Lu compilatore che compila quann ie pare" _BACKTRACE_TRIPLES "/workspace/CMakeLists.txt;52;add_test;/workspace/CMakeLists.txt;0;")
