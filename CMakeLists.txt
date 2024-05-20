# Copyright 2024 Mihail Mladenov
#
# This file is part of PencilAnnealing.
#
# PencilAnnealing is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# PencilAnnealing is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with PencilAnnealing.  If not, see <http://www.gnu.org/licenses/>.


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