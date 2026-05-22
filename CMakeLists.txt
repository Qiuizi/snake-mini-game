cmake_minimum_required(VERSION 3.20)
project(tank_battle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://gitee.com/mirrors/FTXUI.git
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

add_executable(tank_battle
  src/main.cpp
  src/game.cpp
)

target_link_libraries(tank_battle PRIVATE
  ftxui::screen
  ftxui::dom
  ftxui::component
)

target_compile_options(tank_battle PRIVATE -Wall -Wextra)

if(EXISTS "${CMAKE_SOURCE_DIR}/maps")
  add_custom_command(TARGET tank_battle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_SOURCE_DIR}/maps"
      "$<TARGET_FILE_DIR:tank_battle>/maps"
  )
endif()

if(MINGW)
  get_filename_component(MINGW_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  foreach(MINGW_RUNTIME_DLL
      libstdc++-6.dll
      libgcc_s_seh-1.dll
      libwinpthread-1.dll
  )
    add_custom_command(TARGET tank_battle POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MINGW_BIN_DIR}/${MINGW_RUNTIME_DLL}"
        "$<TARGET_FILE_DIR:tank_battle>/${MINGW_RUNTIME_DLL}"
    )
  endforeach()
endif()
