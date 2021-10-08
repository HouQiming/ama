cmake_minimum_required (VERSION 3.0)
project(test)
if(NOT JCC)
  find_program(JCC jcc)
endif()
if(NOT JC_LIB)
  if(DEFINED ENV{JC_LIB})
    set(JC_LIB "$ENV{JC_LIB}")
  else()
    set(JC_LIB "/home/hqm/tp/jc3/lib")
  endif()
endif()
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG=0 -D_GLIBCXX_DEBUG -fno-delete-null-pointer-checks)
endif()
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test_engine.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/test_engine.jc
  MAIN_DEPENDENCY test_engine.jc
  DEPENDS ${JC_LIB}/dumpstack.jch ${JC_LIB}/fs.jch ${JC_LIB}/path.jch ../src/exec/engine.jch ../src/script/jsapi.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/exec/engine.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/exec/engine.jch
  MAIN_DEPENDENCY ../src/exec/engine.jch
  DEPENDS ../src/util/mempool.jch ../src/ast/node.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsapi.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsapi.jch
  MAIN_DEPENDENCY ../src/script/jsapi.jch
  DEPENDS ../src/ast/node.jch ../src/script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/exec/engine.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/exec/engine.jc
  MAIN_DEPENDENCY ../src/exec/engine.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../src/ast/node.jch ../src/script/jsenv.jch ../src/codegen/gen.jch ../src/exec/engine.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsapi.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsapi.jc
  MAIN_DEPENDENCY ../src/script/jsapi.jc
  DEPENDS ${JC_LIB}/json.jch ${JC_LIB}/path.jch ${JC_LIB}/fs.jch ${JC_LIB}/console.jch ${JC_LIB}/env.jch ${JC_LIB}/unicode.jch ${JC_LIB}/unicode/case.jch ../src/ast/node.jch ../src/ast/nodegc.jch ../src/codegen/gen.jch ../src/parser/simppair.jch ../src/parser/scoping.jch ../src/parser/depends.jch ../src/parser/postfix.jch ../src/parser/operator.jch ../src/parser/decl.jch ../src/parser/cleanup.jch ../src/parser/findama.jch ../src/script/nodeof_to_ast.jch ../src/script/jsenv.jch ../src/script/jsapi.jch ../src/script/jsgen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/util/mempool.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/util/mempool.jch
  MAIN_DEPENDENCY ../src/util/mempool.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/node.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/node.jch
  MAIN_DEPENDENCY ../src/ast/node.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsenv.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsenv.jch
  MAIN_DEPENDENCY ../src/script/jsenv.jch
  DEPENDS ../src/ast/node.jch ../src/script/quickjs/src/quickjs.h ../src/script/quickjs/src/cutils.h ../src/script/quickjs/src/list.h ../src/script/quickjs/src/quickjs.h ../src/script/quickjs/src/libregexp.h ../src/script/quickjs/src/libregexp-opcode.h ../src/script/quickjs/src/libunicode.h ../src/script/quickjs/src/libunicode-table.h ../src/script/quickjs/src/quickjs-opcode.h ../src/script/quickjs/src/quickjs-atom.h ../src/script/quickjs/src/quickjs.c ../src/script/quickjs/src/libregexp.c ../src/script/quickjs/src/libunicode.c ../src/script/quickjs/src/cutils.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/util/mempool.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/util/mempool.jc
  MAIN_DEPENDENCY ../src/util/mempool.jc
  DEPENDS ../src/util/mempool.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/node.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/node.jc
  MAIN_DEPENDENCY ../src/ast/node.jc
  DEPENDS ${JC_LIB}/fs.jch ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../src/parser/literal.jch ../src/util/mempool.jch ../src/ast/node.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsenv.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsenv.jc
  MAIN_DEPENDENCY ../src/script/jsenv.jc
  DEPENDS ${JC_LIB}/fs.jch ${JC_LIB}/path.jch ${JC_LIB}/json.jch ../src/script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/json.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/json.jch
  MAIN_DEPENDENCY ${JC_LIB}/json.jch
  DEPENDS ${JC_LIB}/json.cpp ${JC_LIB}/jc_unique_string.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/path.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/path.jch
  MAIN_DEPENDENCY ${JC_LIB}/path.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/fs.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/fs.jch
  MAIN_DEPENDENCY ${JC_LIB}/fs.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/console.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/console.jch
  MAIN_DEPENDENCY ${JC_LIB}/console.jch
  DEPENDS ${JC_LIB}/fix_win_console.cpp ${JC_LIB}/printf.js
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/env.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/env.jch
  MAIN_DEPENDENCY ${JC_LIB}/env.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/unicode.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/unicode.jch
  MAIN_DEPENDENCY ${JC_LIB}/unicode.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/unicode/case.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/unicode/case.jch
  MAIN_DEPENDENCY ${JC_LIB}/unicode/case.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/nodegc.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/nodegc.jch
  MAIN_DEPENDENCY ../src/ast/nodegc.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/codegen/gen.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/codegen/gen.jch
  MAIN_DEPENDENCY ../src/codegen/gen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/simppair.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/simppair.jch
  MAIN_DEPENDENCY ../src/parser/simppair.jch
  DEPENDS ../src/ast/node.jch ../src/script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/scoping.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/scoping.jch
  MAIN_DEPENDENCY ../src/parser/scoping.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/depends.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/depends.jch
  MAIN_DEPENDENCY ../src/parser/depends.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/postfix.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/postfix.jch
  MAIN_DEPENDENCY ../src/parser/postfix.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/operator.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/operator.jch
  MAIN_DEPENDENCY ../src/parser/operator.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/decl.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/decl.jch
  MAIN_DEPENDENCY ../src/parser/decl.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/cleanup.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/cleanup.jch
  MAIN_DEPENDENCY ../src/parser/cleanup.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/findama.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/findama.jch
  MAIN_DEPENDENCY ../src/parser/findama.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/nodeof_to_ast.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/nodeof_to_ast.jch
  MAIN_DEPENDENCY ../src/script/nodeof_to_ast.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsgen.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsgen.jch
  MAIN_DEPENDENCY ../src/script/jsgen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/path_win32.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/path_win32.jc
  MAIN_DEPENDENCY ${JC_LIB}/path_win32.jc
  DEPENDS ${JC_LIB}/path.jch ${JC_LIB}/unicode/case.jch ${JC_LIB}/fs.jch ${JC_LIB}/env.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/path_posix.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/path_posix.jc
  MAIN_DEPENDENCY ${JC_LIB}/path_posix.jc
  DEPENDS ${JC_LIB}/path.jch ${JC_LIB}/fs.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/fs.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/fs.jc
  MAIN_DEPENDENCY ${JC_LIB}/fs.jc
  DEPENDS ${JC_LIB}/unicode.jch ${JC_LIB}/fs.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/env.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/env.jc
  MAIN_DEPENDENCY ${JC_LIB}/env.jc
  DEPENDS ${JC_LIB}/unicode.jch ${JC_LIB}/env.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/unicode.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/unicode.jc
  MAIN_DEPENDENCY ${JC_LIB}/unicode.jc
  DEPENDS ${JC_LIB}/unicode.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/unicode/case.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/unicode/case.jc
  MAIN_DEPENDENCY ${JC_LIB}/unicode/case.jc
  DEPENDS ${JC_LIB}/unicode.jch ${JC_LIB}/unicode/case.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/nodegc.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/ast/nodegc.jc
  MAIN_DEPENDENCY ../src/ast/nodegc.jc
  DEPENDS ../src/ast/node.jch ../src/script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/codegen/gen.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/codegen/gen.jc
  MAIN_DEPENDENCY ../src/codegen/gen.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../src/ast/node.jch ../src/script/jsenv.jch ../src/parser/literal.jch ../src/parser/charset.jch ../src/codegen/gen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/simppair.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/simppair.jc
  MAIN_DEPENDENCY ../src/parser/simppair.jc
  DEPENDS ../src/ast/node.jch ../src/script/jsenv.jch ../src/parser/charset.jch ../src/parser/simppair.jch ${JC_LIB}/console.jch ${JC_LIB}/json.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/scoping.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/scoping.jc
  MAIN_DEPENDENCY ../src/parser/scoping.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../src/ast/node.jch ../src/script/jsenv.jch ../src/parser/scoping.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/depends.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/depends.jc
  MAIN_DEPENDENCY ../src/parser/depends.jc
  DEPENDS ${JC_LIB}/path.jch ${JC_LIB}/fs.jch ../src/ast/node.jch ../src/script/jsenv.jch ../src/parser/depends.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/postfix.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/postfix.jc
  MAIN_DEPENDENCY ../src/parser/postfix.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../src/ast/node.jch ../src/parser/postfix.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/operator.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/operator.jc
  MAIN_DEPENDENCY ../src/parser/operator.jc
  DEPENDS ../src/ast/node.jch ../src/script/jsenv.jch ../src/parser/operator.jch ${JC_LIB}/console.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/decl.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/decl.jc
  MAIN_DEPENDENCY ../src/parser/decl.jc
  DEPENDS ${JC_LIB}/console.jch ../src/ast/node.jch ../src/script/jsenv.jch ../src/parser/postfix.jch ../src/parser/decl.jch ../src/parser/operator.jch ../src/parser/scoping.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/cleanup.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/cleanup.jc
  MAIN_DEPENDENCY ../src/parser/cleanup.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../src/ast/node.jch ../src/parser/cleanup.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/findama.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/findama.jc
  MAIN_DEPENDENCY ../src/parser/findama.jc
  DEPENDS ../src/parser/findama.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/nodeof_to_ast.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/nodeof_to_ast.jc
  MAIN_DEPENDENCY ../src/script/nodeof_to_ast.jc
  DEPENDS ${JC_LIB}/json.jch ../src/script/jsenv.jch ../src/ast/node.jch ../src/script/jsenv.jch ../src/script/nodeof_to_ast.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsgen.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/script/jsgen.jc
  MAIN_DEPENDENCY ../src/script/jsgen.jc
  DEPENDS ../src/script/jsenv.jch ../src/script/jsgen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/literal.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/literal.jch
  MAIN_DEPENDENCY ../src/parser/literal.jch
  DEPENDS ../src/parser/charset.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/literal.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/literal.jc
  MAIN_DEPENDENCY ../src/parser/literal.jc
  DEPENDS ${JC_LIB}/unicode.jch ../src/parser/charset.jch ../src/parser/literal.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/charset.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/charset.jch
  MAIN_DEPENDENCY ../src/parser/charset.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/charset.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../src/parser/charset.jc
  MAIN_DEPENDENCY ../src/parser/charset.jc
  DEPENDS ../src/parser/charset.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
if(DEFINED JC_LIB)
include_directories("${JC_LIB}")
endif()
add_executable(test_engine ${JC_LIB}/dumpstack/linux_bt.cpp ${JC_LIB}/dumpstack/win_dbghelp.cpp ${JC_LIB}/dumpstack/enable_dump.cpp ../src/script/quickjs/src/quickjs.c ../src/script/quickjs/src/libregexp.c ../src/script/quickjs/src/libunicode.c ../src/script/quickjs/src/cutils.c ${JC_LIB}/json.cpp ${JC_LIB}/jc_unique_string.cpp ${JC_LIB}/fix_win_console.cpp ../src/parser/charset.jc.cpp ../src/parser/literal.jc.cpp ../src/script/jsgen.jc.cpp ../src/script/nodeof_to_ast.jc.cpp ../src/parser/findama.jc.cpp ../src/parser/cleanup.jc.cpp ../src/parser/decl.jc.cpp ../src/parser/operator.jc.cpp ../src/parser/postfix.jc.cpp ../src/parser/depends.jc.cpp ../src/parser/scoping.jc.cpp ../src/parser/simppair.jc.cpp ../src/codegen/gen.jc.cpp ../src/ast/nodegc.jc.cpp ${JC_LIB}/unicode/case.jc.cpp ${JC_LIB}/unicode.jc.cpp ${JC_LIB}/env.jc.cpp ${JC_LIB}/fs.jc.cpp ${JC_LIB}/path_posix.jc.cpp ${JC_LIB}/path_win32.jc.cpp ../src/script/jsenv.jc.cpp ../src/ast/node.jc.cpp ../src/util/mempool.jc.cpp ../src/script/jsapi.jc.cpp ../src/exec/engine.jc.cpp test_engine.jc.cpp ../src/ast/node.jch.hpp ../src/parser/charset.jch.hpp ../src/script/jsenv.jch.hpp ../src/util/mempool.jch.hpp ../src/parser/literal.jch.hpp ${JC_LIB}/unicode.jch.hpp ../src/script/jsgen.jch.hpp ../src/script/nodeof_to_ast.jch.hpp ${JC_LIB}/json.jch.hpp ../src/parser/findama.jch.hpp ../src/parser/cleanup.jch.hpp ${JC_LIB}/console.jch.hpp ../src/parser/scoping.jch.hpp ../src/parser/operator.jch.hpp ../src/parser/decl.jch.hpp ../src/parser/postfix.jch.hpp ../src/parser/depends.jch.hpp ${JC_LIB}/fs.jch.hpp ${JC_LIB}/path.jch.hpp ../src/parser/simppair.jch.hpp ../src/codegen/gen.jch.hpp ${JC_LIB}/unicode/case.jch.hpp ${JC_LIB}/env.jch.hpp ../src/script/jsapi.jch.hpp ../src/ast/nodegc.jch.hpp ../src/exec/engine.jch.hpp ${JC_LIB}/dumpstack.jch.hpp)
add_custom_target(run_test_engine
  COMMAND test_engine 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS test_engine
  USES_TERMINAL
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-delete-null-pointer-checks")
include(CheckCXXSourceCompiles)
if(DEFINED JC_LIB)
list(APPEND CMAKE_REQUIRED_INCLUDES "${JC_LIB}")
endif()
check_cxx_source_compiles("#include \"jc_platform.h\"\n\n#if !(JC_OS == JC_OS_WINDOWS)\n#error a\n#endif\nint main(){return 0;}\n" COND_JC_OS_JC_OS_WINDOWS_614D399C)
if(COND_JC_OS_JC_OS_WINDOWS_614D399C)
set_property(
  TARGET test_engine
  APPEND PROPERTY LINK_LIBRARIES
  DbgHelp.lib user32.lib)
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
add_custom_command(
  OUTPUT ${JC_LIB}/dumpstack.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/dumpstack.jch
  MAIN_DEPENDENCY ${JC_LIB}/dumpstack.jch
  DEPENDS ${JC_LIB}/dumpstack/dumpstack.h ${JC_LIB}/dumpstack/linux_bt.cpp ${JC_LIB}/dumpstack/win_dbghelp.cpp ${JC_LIB}/dumpstack/enable_dump.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
check_cxx_source_compiles("#include \"jc_platform.h\"\n\n#if !(!(JC_OS == JC_OS_WINDOWS))\n#error a\n#endif\nint main(){return 0;}\n" COND__JC_OS_JC_OS_WINDOWS__1DB9171C)
if(COND__JC_OS_JC_OS_WINDOWS__1DB9171C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funwind-tables")
endif()
