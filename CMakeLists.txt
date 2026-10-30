cmake_minimum_required(VERSION 3.25)

project(CInterfaceClipsSerializer VERSION 0.1.0 LANGUAGES C)

include(packages.cmake)

add_subdirectory(src)

## Testing

enable_testing()
add_subdirectory(test)
