cmake_minimum_required(VERSION 3.14.0)
project(biolib-ex VERSION 0.0.1 LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
  biolib-lib
  SOURCE_DIR /home/tlemane/work/dev/yoshb/biolib
  GIT_REPOSITORY https://github.com/yhhshb/biolib
  GIT_TAG integration
)

FetchContent_MakeAvailable(biolib-lib)

add_executable(biolib-ex main.cpp)
target_link_libraries(biolib-ex PRIVATE biolib::biolib)

