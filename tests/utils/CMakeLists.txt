add_executable(utils_test utils_test.c)
set_property(TARGET utils_test PROPERTY C_STANDARD 99)
target_compile_options(utils_test PRIVATE -Wall)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/include)

target_link_libraries(utils_test libipa)
if (M32)
  set_target_properties(utils_test PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif()

add_test(NAME utils_test
    COMMAND sh -c "$<TARGET_FILE:utils_test>")

