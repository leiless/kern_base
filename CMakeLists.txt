#
# Created Aug 7, 2020.
# Use `make' to build this project, this CMake file used for CLion code auto-completion.
#

cmake_minimum_required(VERSION 3.0)
project(kern_base C)

set(CMAKE_C_STANDARD 99)

add_compile_options(-Wall -Wextra -g -DDEBUG)

include_directories(
        "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/System/Library/Frameworks/Kernel.framework/Headers"
)

add_library(
        ${PROJECT_NAME}
        src/kern_base.c
)

