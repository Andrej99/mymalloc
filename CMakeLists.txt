cmake_minimum_required(VERSION 3.5)
project(mymalloc)
set(SRC mymalloc.c mymalloc.h main.c)
add_executable(pp ${SRC})