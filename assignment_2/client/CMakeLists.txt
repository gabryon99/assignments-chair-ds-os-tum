cmake_minimum_required(VERSION 3.10)

project(client
    VERSION 0.1
    DESCRIPTION "Client"
) # Create project "client"

option(DEBUG "Enable/disable debug" ON)

set(CMAKE_CXX_STANDARD 20) # Enable C++20 standard

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*;") # Enable clang-tidy

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES ./src/main.cpp)

if(DEBUG)
    add_compile_options(-g -O1)
else()
    add_compile_options(-O3 -Wall -pedantic)
endif()

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(client ${SOURCE_FILES})

target_include_directories(client PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ./include/
    ../common/include/)