add_subdirectory(libasn)
add_subdirectory(libipa)

add_library(http STATIC http.c)
add_library(scard STATIC scard.c)

target_include_directories(http PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_include_directories(scard PUBLIC ${CMAKE_SOURCE_DIR}/include)

include_directories(include /usr/include/PCSC)
add_executable(ipa main.c)
set_property(TARGET ipa PROPERTY C_STANDARD 99)
target_compile_options(ipa PRIVATE -Wall)

target_include_directories(ipa PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ipa libipa http scard pcsclite curl)

