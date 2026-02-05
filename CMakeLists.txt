cmake_minimum_required(VERSION 3.10)

project(auc)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
   uiohook
   GIT_REPOSITORY https://github.com/kwhat/libuiohook.git
   GIT_TAG        1.2
)
FetchContent_MakeAvailable(uiohook)

add_executable(auc "src/main.c")
target_link_options(auc PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(auc PRIVATE uiohook)
