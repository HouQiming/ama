cmake_minimum_required (VERSION 3.0)
project(entry)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-delete-null-pointer-checks")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
if(CMAKE_BUILD_TYPE MATCHES DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG=0 -D_GLIBCXX_DEBUG")
endif()
add_executable(ama
	"src/util/dumpstack/linux_bt.cpp"
	"src/util/dumpstack/win_dbghelp.cpp"
	"src/util/dumpstack/enable_dump.cpp"
	"src/util/fix_win_console.cpp"
	"src/entry/ama.cpp")
add_library(amal SHARED
	"src/script/quickjs/src/quickjs.c"
	"src/script/quickjs/src/libregexp.c"
	"src/script/quickjs/src/libunicode.c"
	"src/script/quickjs/src/cutils.c"
	"modules/cpp/json/json.cpp"
	"src/util/jc_unique_string.cpp"
	"src/util/fix_win_console.cpp"
	"src/parser/charset.cpp"
	"src/util/mempool.cpp"
	"src/parser/literal.cpp"
	"src/script/jsgen.cpp"
	"src/script/nodeof_to_ast.cpp"
	"src/parser/findama.cpp"
	"src/parser/cleanup.cpp"
	"src/parser/decl.cpp"
	"src/parser/operator.cpp"
	"src/parser/postfix.cpp"
	"src/parser/depends.cpp"
	"src/parser/scoping.cpp"
	"src/parser/simppair.cpp"
	"src/codegen/gen.cpp"
	"src/ast/nodegc.cpp"
	"src/util/unicode/case.cpp"
	"src/util/unicode.cpp"
	"src/util/env.cpp"
	"src/util/fs.cpp"
	"src/util/path_posix.cpp"
	"src/util/path_win32.cpp"
	"src/script/jsenv.cpp"
	"src/ast/node.cpp"
	"src/script/jsapi.cpp"
	"src/entry/amal.cpp")
set_property(
	TARGET ama
	APPEND PROPERTY LINK_LIBRARIES
	amal)
if(WIN32)
	set_property(
		TARGET ama
		APPEND PROPERTY LINK_LIBRARIES
		DbgHelp.lib user32.lib)
	set_property(
		TARGET amal
		APPEND PROPERTY LINK_LIBRARIES
		kernel32.lib user32.lib)
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funwind-tables")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
	set_property(
      TARGET amal
      APPEND PROPERTY LINK_LIBRARIES
      -ldl)
	set_property(
      TARGET ama
      APPEND PROPERTY LINK_LIBRARIES
      -ldl)
endif()
