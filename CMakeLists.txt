cmake_minimum_required (VERSION 3.0)
project(ama)
if(MSVC)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4068 /D_CRT_SECURE_NO_WARNINGS")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4068 /EHsc /std:c++latest /D_CRT_SECURE_NO_WARNINGS")
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-delete-null-pointer-checks")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-delete-null-pointer-checks")
endif()
add_executable(ama
	"src/util/dumpstack/linux_bt.cpp"
	"src/util/dumpstack/win_dbghelp.cpp"
	"src/util/dumpstack/enable_dump.cpp"
	"src/entry/ama.cpp")
add_library(amal SHARED
	"src/script/quickjs/src/quickjs.c"
	"src/script/quickjs/src/libregexp.c"
	"src/script/quickjs/src/libunicode.c"
	"src/script/quickjs/src/cutils.c"
	"modules/cpp/json/json.cpp"
	"src/util/gcstring.cpp"
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
	"src/script/jsapi.cpp")
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
if(MSVC)
set_target_properties(amal PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)
endif()
else()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funwind-tables")
set_property(
  TARGET amal
  APPEND PROPERTY LINK_LIBRARIES
  -ldl)
set_property(
  TARGET ama
  APPEND PROPERTY LINK_LIBRARIES
  -ldl)
endif()

install(TARGETS ama DESTINATION bin)
install(TARGETS amal DESTINATION lib)
#install(DIRECTORY modules/ DESTINATION share/ama_modules)
