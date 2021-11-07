cmake_minimum_required(VERSION 3.10)
project(Support C)

set(CMAKE_C_STANDARD 11)

include_directories(.)
include_directories(queue)

add_executable(Support
        queue/queue.h
        sut.c
        sut.h
        test1.c
        test2.c
        test3.c
        test4.c
        test5.c)
