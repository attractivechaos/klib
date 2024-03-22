cmake_minimum_required(VERSION 3.19)
set(CMAKE_C_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# SDK path
set(ENVSDK $ENV{PLAYDATE_SDK_PATH})
if(NOT ${ENVSDK} STREQUAL "")
    # Convert path from Windows
    file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
    execute_process(
        COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
        COMMAND head -n 1
        COMMAND cut -c9-
        OUTPUT_VARIABLE SDK
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
if(NOT EXISTS ${SDK})
    message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
    return()
endif()

set(LIB_NAME "klib")
project(${LIB_NAME} C ASM)
set(PLAYDATE_GAME_NAME ${LIB_NAME})
set(PLAYDATE_GAME_DEVICE ${LIB_NAME})

message(STATUS "CMAKE_GENERATOR: ${CMAKE_GENERATOR}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
if(NOT ${TOOLCHAIN} STREQUAL "")
    message(STATUS "TOOLCHAIN: ${TOOLCHAIN}")
else()
    message(STATUS "TOOLCHAIN: clang")
endif()


set(SOURCES "kstring.c")
add_library(${LIB_NAME} STATIC ${SOURCES})

include(${SDK}/C_API/buildsupport/playdate.cmake)

