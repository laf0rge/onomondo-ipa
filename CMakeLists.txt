cmake_minimum_required(VERSION 3.13...3.19 FATAL_ERROR)
project(ipa VERSION 1.0.0 LANGUAGES C)

option(ENABLE_SANITIZE "Compile with address sanitizer enabled" OFF)

if (ENABLE_SANITIZE)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

add_subdirectory(src)

include(CTest)
add_subdirectory(tests)
