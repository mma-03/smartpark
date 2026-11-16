# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(my_hello_world VERSION 1.0 DESCRIPTION "Starter project" LANGUAGES C)

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# Enable PThread library for linking
add_compile_options(-pthread)
add_link_options(-pthread)

# What folders to build
add_subdirectory(hal)  
add_subdirectory(app)

# build rotary encoder test program
add_executable(rotary_test tests/rotary_test.c)
target_link_libraries(rotary_test PRIVATE hal)

# build joystick test program
add_executable(joystick_test tests/joystick_test.c)
target_link_libraries(joystick_test PRIVATE hal)

# build local controls test program
add_executable(local_controls_test tests/local_controls_test.c app/src/local_controls.c)
target_include_directories(local_controls_test PRIVATE app/include)
target_link_libraries(local_controls_test PRIVATE hal)

