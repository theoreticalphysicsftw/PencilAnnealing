cmake_minimum_required(VERSION 3.5) 

project(theora)
enable_language(C)

file(GLOB THEORA_SOURCES "lib/*.c")
    
add_library(${PROJECT_NAME} "${THEORA_SOURCES}")
target_include_directories(${PROJECT_NAME} PUBLIC "include")