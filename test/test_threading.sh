#!/bin/sh
c++ -O2 -otest_threading.exe test_threading.cpp -L${HOME}/.local/lib_debug -lamal -lpthread || exit
LD_LIBRARY_PATH=~/.local/lib_debug/ ./test_threading.exe
