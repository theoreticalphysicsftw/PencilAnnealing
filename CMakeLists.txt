# MIT License
# 
# Copyright (c) 2024 Mihail Mladenov
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


cmake_minimum_required(VERSION 3.5) 

project(PencilAnnealing)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(
        GLOB
        PA_SOURCES
        "src/*.cpp"
    )

file(
        GLOB
        LIBWEBP_SOURCES
        "extern/libwebp/src/enc/*.c"
        "extern/libwebp/src/dec/*.c"
        "extern/libwebp/src/dsp/*.c"
        "extern/libwebp/src/mux/*.c"
        "extern/libwebp/src/demux/*.c"
        "extern/libwebp/src/utils/*.c"
        "extern/libwebp/sharpyuv/*.c"
    )

add_executable(${PROJECT_NAME} ${PA_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC "extern")

target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:PA_DEBUG>)
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<CONFIG:Release>:PA_RELEASE>)

add_subdirectory("extern/SDL2")
target_link_libraries(${PROJECT_NAME} SDL2-static)

add_subdirectory("extern/libtheora")
target_link_libraries(${PROJECT_NAME} theora)
target_include_directories(${PROJECT_NAME} PUBLIC "extern/libtheora/include")

add_library(libwebp ${LIBWEBP_SOURCES})
target_include_directories(libwebp PUBLIC "extern/libwebp")
target_link_libraries(${PROJECT_NAME} libwebp)

file(
        GLOB
        PA_TESTS
        "test/*.cpp"
    )

foreach(TEST_SOURCE ${PA_TESTS})
    cmake_path(GET TEST_SOURCE STEM TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_include_directories(${TEST_NAME} PUBLIC "src")
    target_include_directories(${TEST_NAME} PUBLIC "extern")
    target_include_directories(${TEST_NAME} PUBLIC "extern/libtheora/include")
endforeach()