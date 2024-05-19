# Easy CMake build of libtheora

This is simplified multiplatform build of libtheora using CMake. Everything but the source files and the original copyright notice is removed. Assembly accelerated functions are removed (the x86 code is worthless anyway if the goal is to have x86_64 binaries). The `liboog` source files are already included (no external dependency).


### Building

To add to existing cmake project just:

```
add_subdirectory("path_to_libtheora_cmake")
target_link_libraries(${PROJECT_NAME} theora)
target_include_directories(${PROJECT_NAME} PUBLIC "path_to_lib_theora_cmake/include")
```

For standalone build on Linux:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j `nproc`
```

For standalone build on Windows:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config=Release -j %NUMBER_OF_PROCESSORS%
```
