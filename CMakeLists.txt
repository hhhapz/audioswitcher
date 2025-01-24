cmake_minimum_required(VERSION 3.16) 
project(AudioSwitcher)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PULSE REQUIRED libpulse-simple)

include_directories(${PULSE_INCLUDE_DIRS})
link_directories(${PULSE_LIBRARY_DIRS})
add_definitions(${PULSE_CFLAGS_OTHER})

###
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}")

add_subdirectory(deps)
add_executable(audioswitcher main.cpp audioswitcher.h audioswitcher.cpp)

target_link_libraries(audioswitcher ${PULSE_LIBRARIES} optionparser)

