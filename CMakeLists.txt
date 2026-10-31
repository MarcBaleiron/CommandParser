cmake_minimum_required(VERSION 3.16)
project(CommandParser C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror -Wpedantic)

add_library(core src/crc16.c src/packet.c src/ring_buffer.c)
target_include_directories(core PUBLIC include)

add_executable(CommandParser src/main.c)
target_link_libraries(CommandParser core)

enable_testing()
add_executable(tests_crc tests/tests_crc.c)
target_link_libraries(tests_crc core)
add_test(NAME crc COMMAND tests_crc)

add_executable(tests_packet tests/tests_packet.c)
target_link_libraries(tests_packet core)
add_test(NAME packet COMMAND tests_packet)

add_executable(tests_rb tests/tests_rb.c)
target_link_libraries(tests_rb core)
add_test(NAME rb COMMAND tests_rb)