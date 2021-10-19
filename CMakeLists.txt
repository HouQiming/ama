cmake_minimum_required (VERSION 3.0)
project(entry)
set(JC_LIB "${CMAKE_CURRENT_SOURCE_DIR}/jc_lib")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-delete-null-pointer-checks")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG=0 -D_GLIBCXX_DEBUG")
endif()
if(DEFINED JC_LIB)
include_directories("${JC_LIB}")
endif()
add_executable(ama "${JC_LIB}/dumpstack/linux_bt.cpp" "${JC_LIB}/dumpstack/win_dbghelp.cpp" "${JC_LIB}/dumpstack/enable_dump.cpp" "${JC_LIB}/fix_win_console.cpp" "src/entry/ama.cpp" "${JC_LIB}/console.hpp" "${JC_LIB}/dumpstack.hpp")
add_library(amal SHARED "src/script/quickjs/src/quickjs.c" "src/script/quickjs/src/libregexp.c" "src/script/quickjs/src/libunicode.c" "src/script/quickjs/src/cutils.c" "../ama/modules/cpp/json/json.cpp" "../ama/src/util/jc_unique_string.cpp" "${JC_LIB}/fix_win_console.cpp" "src/parser/charset.cpp" "src/util/mempool.cpp" "src/parser/literal.cpp" "src/script/jsgen.cpp" "src/script/nodeof_to_ast.cpp" "src/exec/engine.cpp" "src/parser/findama.cpp" "src/parser/cleanup.cpp" "src/parser/decl.cpp" "src/parser/operator.cpp" "src/parser/postfix.cpp" "src/parser/depends.cpp" "src/parser/scoping.cpp" "src/parser/simppair.cpp" "src/codegen/gen.cpp" "src/ast/nodegc.cpp" "${JC_LIB}/unicode/case.cpp" "${JC_LIB}/unicode.cpp" "${JC_LIB}/env.cpp" "${JC_LIB}/fs.cpp" "${JC_LIB}/path_posix.cpp" "${JC_LIB}/path_win32.cpp" "src/script/jsenv.cpp" "src/ast/node.cpp" "src/script/jsapi.cpp" "src/entry/amal.cpp" "src/ast/node.hpp" "src/parser/charset.hpp" "src/script/jsenv.hpp" "src/util/mempool.hpp" "src/parser/literal.hpp" "src/entry/ama.cpp" "${JC_LIB}/unicode.hpp" "src/script/jsgen.hpp" "src/script/nodeof_to_ast.hpp" "src/exec/engine.hpp" "src/parser/findama.hpp" "src/parser/cleanup.hpp" "${JC_LIB}/console.hpp" "src/parser/scoping.hpp" "src/parser/operator.hpp" "src/parser/decl.hpp" "src/parser/postfix.hpp" "src/parser/depends.hpp" "${JC_LIB}/fs.hpp" "${JC_LIB}/path.hpp" "src/parser/simppair.hpp" "src/codegen/gen.hpp" "${JC_LIB}/unicode/case.hpp" "${JC_LIB}/env.hpp" "src/script/jsapi.hpp" "src/ast/nodegc.hpp")
set_property(
  TARGET ama
  APPEND PROPERTY LINK_LIBRARIES
  amal)
if(DEFINED JC_LIB)
list(APPEND CMAKE_REQUIRED_INCLUDES "${JC_LIB}")
endif()
if(WIN32)
set_property(
  TARGET ama
  APPEND PROPERTY LINK_LIBRARIES
  DbgHelp.lib user32.lib)
set_property(
  TARGET amal
  APPEND PROPERTY LINK_LIBRARIES
  user32.lib
)
endif()
if(NOT WIN32)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funwind-tables")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
set_property(
  TARGET ama
  APPEND PROPERTY LINK_LIBRARIES
  -ldl
)
endif()
if(NOT WIN32)
set_property(
  TARGET amal
  APPEND PROPERTY LINK_LIBRARIES
  -ldl
)
endif()
if(WIN32)
set_property(
  TARGET amal
  APPEND PROPERTY LINK_LIBRARIES
  kernel32.lib
)
endif()
if(COND_DEFINED_WIN32__A386370D)
set_property(
  TARGET amal
  APPEND PROPERTY LINK_LIBRARIES
  kernel32.lib
)
endif()
if(COND__DEFINED_WIN32__36BC7958)
set_property(
  TARGET amal
  APPEND PROPERTY LINK_LIBRARIES
  -ldl
)
endif()
