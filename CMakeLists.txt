cmake_minimum_required(VERSION 3.10)

project(auc)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(uiohook)

add_executable(auc "src/main.c")
target_link_options(auc PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(auc PRIVATE uiohook)
