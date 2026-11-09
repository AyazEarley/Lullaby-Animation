cmake_minimum_required(VERSION 3.16)
project(pointviz CXX)
set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5)
FetchContent_MakeAvailable(raylib)

add_executable(pointviz src/main.cpp)
target_link_libraries(pointviz PRIVATE raylib)