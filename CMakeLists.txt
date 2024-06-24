cmake_minimum_required(VERSION 3.13...3.19 FATAL_ERROR)
project(ipa VERSION 1.0.0 LANGUAGES C)

option(ENABLE_SANITIZE "Compile with address sanitizer enabled" OFF)
option(SHOW_ASN1_OUTPUT "Show ASN.1 decoded output in the log" OFF)
option(ASN_EMIT_DEBUG "Enable built-in debug output of asn1c" OFF)

if (ENABLE_SANITIZE)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

if(SHOW_ASN1_OUTPUT)
  add_definitions(-DSHOW_ASN1_OUTPUT)
endif()

if(ASN_EMIT_DEBUG)
  add_definitions(-DASN_EMIT_DEBUG)
endif()

add_subdirectory(src)

include(CTest)
add_subdirectory(tests)
