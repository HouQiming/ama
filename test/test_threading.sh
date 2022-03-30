#!/bin/sh
c++ -O2 -otest_threading.exe test_threading.cpp -lamal -lpthread || exit
#LD_LIBRARY_PATH=../build/linux_debug/
./test_threading.exe
