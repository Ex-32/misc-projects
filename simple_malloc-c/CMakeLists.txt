cmake_minimum_required(VERSION 3.11)
project(simple_malloc VERSION 1.0.0 DESCRIPTION "A simple memory allocator like malloc")
add_library(simple_malloc
    "simple_malloc.c"
)
set_target_properties(simple_malloc PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION 1
    PUBLIC_HEADER "simple_malloc.h"
)
target_include_directories(simple_malloc PRIVATE ./src)
