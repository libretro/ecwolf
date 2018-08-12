# This file provides a config for find_package so that we can use our vendored
# libraries or the system libraries with minimal boilerplate code.
add_library(SDL2::SDL2 ALIAS SDL2-static)
