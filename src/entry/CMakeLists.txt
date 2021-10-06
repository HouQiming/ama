cmake_minimum_required (VERSION 3.0)
project(entry)
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
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-delete-null-pointer-checks")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG=0 -D_GLIBCXX_DEBUG")
endif()
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/ama.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/ama.jc
  MAIN_DEPENDENCY ama.jc
  DEPENDS ${JC_LIB}/dumpstack.jch ${JC_LIB}/path.jch ${JC_LIB}/fs.jch ${JC_LIB}/console.jch ${JC_LIB}/json.jch ${JC_LIB}/unicode.jch ../parser/findama.jch ../script/jsapi.jch ../script/jsenv.jch
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
  OUTPUT ${JC_LIB}/json.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/json.jch
  MAIN_DEPENDENCY ${JC_LIB}/json.jch
  DEPENDS ${JC_LIB}/json.cpp ${JC_LIB}/jc_unique_string.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/findama.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/findama.jch
  MAIN_DEPENDENCY ../parser/findama.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsapi.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsapi.jch
  MAIN_DEPENDENCY ../script/jsapi.jch
  DEPENDS ../ast/node.jch ../script/jsenv.jch
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
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/findama.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/findama.jc
  MAIN_DEPENDENCY ../parser/findama.jc
  DEPENDS ../parser/findama.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsapi.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsapi.jc
  MAIN_DEPENDENCY ../script/jsapi.jc
  DEPENDS ${JC_LIB}/json.jch ${JC_LIB}/path.jch ${JC_LIB}/fs.jch ${JC_LIB}/console.jch ${JC_LIB}/env.jch ${JC_LIB}/unicode.jch ${JC_LIB}/unicode/case.jch ../ast/node.jch ../ast/nodegc.jch ../codegen/gen.jch ../parser/simppair.jch ../parser/scoping.jch ../parser/depends.jch ../parser/postfix.jch ../parser/operator.jch ../parser/decl.jch ../parser/cleanup.jch ../script/nodeof_to_ast.jch ../script/jsenv.jch ../script/jsapi.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/unicode/case.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/unicode/case.jch
  MAIN_DEPENDENCY ${JC_LIB}/unicode/case.jch
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
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsenv.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsenv.jch
  MAIN_DEPENDENCY ../script/jsenv.jch
  DEPENDS ../ast/node.jch ../script/quickjs/src/quickjs.h ../script/quickjs/src/cutils.h ../script/quickjs/src/list.h ../script/quickjs/src/quickjs.h ../script/quickjs/src/libregexp.h ../script/quickjs/src/libregexp-opcode.h ../script/quickjs/src/libunicode.h ../script/quickjs/src/libunicode-table.h ../script/quickjs/src/quickjs-opcode.h ../script/quickjs/src/quickjs-atom.h ../script/quickjs/src/quickjs.c ../script/quickjs/src/libregexp.c ../script/quickjs/src/libunicode.c ../script/quickjs/src/cutils.c
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
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsenv.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../script/jsenv.jc
  MAIN_DEPENDENCY ../script/jsenv.jc
  DEPENDS ${JC_LIB}/fs.jch ${JC_LIB}/path.jch ${JC_LIB}/json.jch ../script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
if(DEFINED JC_LIB)
include_directories("${JC_LIB}")
endif()
add_executable(ama ${JC_LIB}/dumpstack/linux_bt.cpp ${JC_LIB}/dumpstack/win_dbghelp.cpp ${JC_LIB}/dumpstack/enable_dump.cpp ${JC_LIB}/fix_win_console.cpp ${JC_LIB}/json.cpp ${JC_LIB}/jc_unique_string.cpp ../script/quickjs/src/quickjs.c ../script/quickjs/src/libregexp.c ../script/quickjs/src/libunicode.c ../script/quickjs/src/cutils.c ../parser/charset.jc.cpp ../util/mempool.jc.cpp ../parser/literal.jc.cpp ../script/nodeof_to_ast.jc.cpp ../parser/cleanup.jc.cpp ../parser/decl.jc.cpp ../parser/operator.jc.cpp ../parser/postfix.jc.cpp ../parser/depends.jc.cpp ../parser/scoping.jc.cpp ../parser/simppair.jc.cpp ../codegen/gen.jc.cpp ../ast/nodegc.jc.cpp ../ast/node.jc.cpp ../script/jsenv.jc.cpp ${JC_LIB}/unicode.jc.cpp ${JC_LIB}/env.jc.cpp ${JC_LIB}/unicode/case.jc.cpp ../script/jsapi.jc.cpp ../parser/findama.jc.cpp ${JC_LIB}/fs.jc.cpp ${JC_LIB}/path_posix.jc.cpp ${JC_LIB}/path_win32.jc.cpp ama.jc.cpp ../parser/charset.jch.hpp ../util/mempool.jch.hpp ../parser/literal.jch.hpp ../script/nodeof_to_ast.jch.hpp ../parser/cleanup.jch.hpp ../parser/operator.jch.hpp ../parser/postfix.jch.hpp ../parser/simppair.jch.hpp ../script/jsenv.jch.hpp ../ast/node.jch.hpp ${JC_LIB}/unicode.jch.hpp ../codegen/gen.jch.hpp ../ast/nodegc.jch.hpp ${JC_LIB}/env.jch.hpp ${JC_LIB}/unicode/case.jch.hpp ../script/jsapi.jch.hpp ../parser/decl.jch.hpp ../parser/scoping.jch.hpp ${JC_LIB}/fs.jch.hpp ${JC_LIB}/path.jch.hpp ${JC_LIB}/json.jch.hpp ../parser/depends.jch.hpp ../parser/findama.jch.hpp ${JC_LIB}/console.jch.hpp ${JC_LIB}/dumpstack.jch.hpp)
include(CheckCXXSourceCompiles)
if(DEFINED JC_LIB)
list(APPEND CMAKE_REQUIRED_INCLUDES "${JC_LIB}")
endif()
check_cxx_source_compiles("#include \"jc_platform.h\"\n\n#if !(JC_OS == JC_OS_WINDOWS)\n#error a\n#endif\nint main(){return 0;}\n" COND_JC_OS_JC_OS_WINDOWS_614D399C)
if(COND_JC_OS_JC_OS_WINDOWS_614D399C)
set_property(
  TARGET ama
  APPEND PROPERTY LINK_LIBRARIES
  DbgHelp.lib user32.lib)
endif()
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
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
endif()
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../ast/node.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../ast/node.jch
  MAIN_DEPENDENCY ../ast/node.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/simppair.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/simppair.jch
  MAIN_DEPENDENCY ../parser/simppair.jch
  DEPENDS ../ast/node.jch ../script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../ast/node.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../ast/node.jc
  MAIN_DEPENDENCY ../ast/node.jc
  DEPENDS ${JC_LIB}/fs.jch ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../parser/literal.jch ../util/mempool.jch ../ast/node.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/simppair.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/simppair.jc
  MAIN_DEPENDENCY ../parser/simppair.jc
  DEPENDS ../ast/node.jch ../script/jsenv.jch ../parser/charset.jch ../parser/simppair.jch ${JC_LIB}/console.jch ${JC_LIB}/json.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/literal.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/literal.jch
  MAIN_DEPENDENCY ../parser/literal.jch
  DEPENDS ../parser/charset.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/literal.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/literal.jc
  MAIN_DEPENDENCY ../parser/literal.jc
  DEPENDS ${JC_LIB}/unicode.jch ../parser/charset.jch ../parser/literal.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/charset.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/charset.jch
  MAIN_DEPENDENCY ../parser/charset.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/charset.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/charset.jc
  MAIN_DEPENDENCY ../parser/charset.jc
  DEPENDS ../parser/charset.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../codegen/gen.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../codegen/gen.jch
  MAIN_DEPENDENCY ../codegen/gen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../codegen/gen.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../codegen/gen.jc
  MAIN_DEPENDENCY ../codegen/gen.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../ast/node.jch ../script/jsenv.jch ../parser/literal.jch ../parser/charset.jch ../codegen/gen.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/scoping.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/scoping.jch
  MAIN_DEPENDENCY ../parser/scoping.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/scoping.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/scoping.jc
  MAIN_DEPENDENCY ../parser/scoping.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../ast/node.jch ../script/jsenv.jch ../parser/scoping.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/postfix.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/postfix.jch
  MAIN_DEPENDENCY ../parser/postfix.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/postfix.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/postfix.jc
  MAIN_DEPENDENCY ../parser/postfix.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../ast/node.jch ../parser/postfix.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/depends.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/depends.jch
  MAIN_DEPENDENCY ../parser/depends.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/depends.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/depends.jc
  MAIN_DEPENDENCY ../parser/depends.jc
  DEPENDS ${JC_LIB}/path.jch ${JC_LIB}/fs.jch ../ast/node.jch ../script/jsenv.jch ../parser/depends.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/cleanup.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/cleanup.jch
  MAIN_DEPENDENCY ../parser/cleanup.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/cleanup.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/cleanup.jc
  MAIN_DEPENDENCY ../parser/cleanup.jc
  DEPENDS ${JC_LIB}/console.jch ${JC_LIB}/json.jch ../ast/node.jch ../parser/cleanup.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../script/nodeof_to_ast.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../script/nodeof_to_ast.jch
  MAIN_DEPENDENCY ../script/nodeof_to_ast.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../script/nodeof_to_ast.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../script/nodeof_to_ast.jc
  MAIN_DEPENDENCY ../script/nodeof_to_ast.jc
  DEPENDS ${JC_LIB}/json.jch ../script/jsenv.jch ../ast/node.jch ../script/jsenv.jch ../script/nodeof_to_ast.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/operator.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/operator.jch
  MAIN_DEPENDENCY ../parser/operator.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/operator.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/operator.jc
  MAIN_DEPENDENCY ../parser/operator.jc
  DEPENDS ../ast/node.jch ../script/jsenv.jch ../parser/operator.jch ${JC_LIB}/console.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/decl.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/decl.jch
  MAIN_DEPENDENCY ../parser/decl.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../parser/decl.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../parser/decl.jc
  MAIN_DEPENDENCY ../parser/decl.jc
  DEPENDS ${JC_LIB}/console.jch ../ast/node.jch ../script/jsenv.jch ../parser/postfix.jch ../parser/decl.jch ../parser/operator.jch ../parser/scoping.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../ast/nodegc.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../ast/nodegc.jch
  MAIN_DEPENDENCY ../ast/nodegc.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../ast/nodegc.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../ast/nodegc.jc
  MAIN_DEPENDENCY ../ast/nodegc.jc
  DEPENDS ../ast/node.jch ../script/jsenv.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../util/mempool.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../util/mempool.jch
  MAIN_DEPENDENCY ../util/mempool.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/../util/mempool.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/../util/mempool.jc
  MAIN_DEPENDENCY ../util/mempool.jc
  DEPENDS ../util/mempool.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
