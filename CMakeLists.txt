cmake_minimum_required(VERSION 3.16)
project(CommandParser C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -Wpedantic)

add_library(core src/crc16.c src/packet.c)
target_include_directories(core PUBLIC include)

add_executable(CommandParser src/main.c)
target_link_libraries(CommandParser core)

enable_testing()
add_executable(test_crc tests/test_crc.c)
target_link_libraries(test_crc core)
add_test(NAME crc COMMAND test_crc)