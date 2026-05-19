cmake_minimum_required(VERSION 3.20)
project(snake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://gitee.com/mirrors/FTXUI.git
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

add_executable(snake
  src/main.cpp
  src/game.cpp
)

target_link_libraries(snake PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
)

target_compile_options(snake PRIVATE -Wall -Wextra)
