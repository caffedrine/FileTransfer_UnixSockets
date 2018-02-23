cmake_minimum_required(VERSION 3.8)
project(Client)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        client.c)

add_executable(Client ${SOURCE_FILES})