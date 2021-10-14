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
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_LIBCPP_DEBUG=0 -D_GLIBCXX_DEBUG")
endif()
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test_engine.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${CMAKE_CURRENT_SOURCE_DIR}/test_engine.jc
  MAIN_DEPENDENCY test_engine.jc
  DEPENDS ${JC_LIB}/dumpstack.jch ${JC_LIB}/fs.jch ${JC_LIB}/path.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/dumpstack.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/dumpstack.jch
  MAIN_DEPENDENCY ${JC_LIB}/dumpstack.jch
  DEPENDS ${JC_LIB}/dumpstack/dumpstack.h ${JC_LIB}/dumpstack/linux_bt.cpp ${JC_LIB}/dumpstack/win_dbghelp.cpp ${JC_LIB}/dumpstack/enable_dump.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/fs.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/fs.jch
  MAIN_DEPENDENCY ${JC_LIB}/fs.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_custom_command(
  OUTPUT ${JC_LIB}/path.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/path.jch
  MAIN_DEPENDENCY ${JC_LIB}/path.jch
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
  OUTPUT ${JC_LIB}/env.jch.hpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/env.jch
  MAIN_DEPENDENCY ${JC_LIB}/env.jch
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
  OUTPUT ${JC_LIB}/env.jc.cpp
  COMMAND ${JCC} --cmakelist ${CMAKE_CURRENT_LIST_FILE} -c ${JC_LIB}/env.jc
  MAIN_DEPENDENCY ${JC_LIB}/env.jc
  DEPENDS ${JC_LIB}/unicode.jch ${JC_LIB}/env.jch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
if(DEFINED JC_LIB)
include_directories("${JC_LIB}")
endif()
add_custom_target(run_test_engine
  COMMAND test_engine 
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS test_engine
  USES_TERMINAL
)
add_executable(test_engine ${JC_LIB}/dumpstack/linux_bt.cpp ${JC_LIB}/dumpstack/win_dbghelp.cpp ${JC_LIB}/dumpstack/enable_dump.cpp ${JC_LIB}/env.jc.cpp ${JC_LIB}/unicode/case.jc.cpp ${JC_LIB}/unicode.jc.cpp ${JC_LIB}/path_posix.jc.cpp ${JC_LIB}/path_win32.jc.cpp ${JC_LIB}/fs.jc.cpp test_engine.jc.cpp ${JC_LIB}/env.jch.hpp ${JC_LIB}/unicode.jch.hpp ${JC_LIB}/unicode/case.jch.hpp ${JC_LIB}/fs.jch.hpp ${JC_LIB}/path.jch.hpp ${JC_LIB}/dumpstack.jch.hpp)
include(CheckCXXSourceCompiles)
if(DEFINED JC_LIB)
list(APPEND CMAKE_REQUIRED_INCLUDES "${JC_LIB}")
endif()
check_cxx_source_compiles("#include \"jc_platform.h\"\n\n#if !(!(JC_OS == JC_OS_WINDOWS))\n#error a\n#endif\nint main(){return 0;}\n" COND__JC_OS_JC_OS_WINDOWS__1DB9171C)
if(COND__JC_OS_JC_OS_WINDOWS__1DB9171C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funwind-tables")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -funwind-tables")
set_property(
  TARGET test_engine
  APPEND PROPERTY LINK_LIBRARIES
  -ldl
)
endif()
set_property(
  TARGET test_engine
  APPEND PROPERTY LINK_LIBRARIES
  -lamal
)
check_cxx_source_compiles("#include \"jc_platform.h\"\n\n#if !(JC_OS == JC_OS_WINDOWS)\n#error a\n#endif\nint main(){return 0;}\n" COND_JC_OS_JC_OS_WINDOWS_614D399C)
if(COND_JC_OS_JC_OS_WINDOWS_614D399C)
set_property(
  TARGET test_engine
  APPEND PROPERTY LINK_LIBRARIES
  DbgHelp.lib user32.lib
)
endif()
set_property(
  TARGET test_engine
  APPEND PROPERTY LINK_DIRECTORIES
  ~/.local/lib
)
add_library(native_module SHARED native_module.cpp)
