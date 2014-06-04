cmake_minimum_required(VERSION 2.8.11)
project(klib-test)
enable_testing()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(Threads REQUIRED)
find_package(Check REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99 -Wall -Wextra -Werror -pedantic -Wstrict-prototypes -Wold-style-definition")

add_executable(klib-test main klist_tests)
target_include_directories(klib-test PUBLIC ${CHECK_INCLUDE_DIRS} ../)
target_link_libraries(klib-test ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_test(klib-test ${CMAKE_CURRENT_BINARY_DIR}/klib-test)
