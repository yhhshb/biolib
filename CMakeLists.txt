cmake_minimum_required(VERSION 3.11)
project(BIOLIB)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

MESSAGE(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (UNIX AND NOT APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2 -msse4.2") # for hardware popcount and pdep
endif()

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-char-subscripts")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
  if (BIOLIB_USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  endif()
endif()

# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

set(BIOLIB_SOURCES
  src/io.cpp
  src/hash.cpp
  bundled/MurmurHash3.cpp
)

# Tests------------------------------------------------------------------------
if (MAKE_TESTS)
  add_executable(emv_test tests/external_memory_vector.cpp)
  target_link_libraries(emv_test argparse)
  add_executable(ous_test tests/ordered_unique_sampler.cpp)
  target_link_libraries(ous_test argparse)
  add_executable(kv_test tests/kmer_view.cpp)
  target_link_libraries(kv_test argparse z)
  add_executable(jaccard_test tests/jaccard.cpp)
  target_link_libraries(jaccard_test argparse z)
  add_executable(rsg tests/random_sequence_generation.cpp)
  target_link_libraries(rsg argparse)

  add_executable(bv_test tests/bit_vector.cpp)
  add_executable(pv_test tests/packed_vector.cpp)
  add_executable(rs_test tests/rs.cpp)

  add_executable(popcount tests/popcount.cpp)
  add_executable(traits_examples tests/traits_examples.cpp bundled/MurmurHash3.cpp)
endif()