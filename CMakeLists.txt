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

if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-sign-compare")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2 -msse4.2") # for hardware popcount and pdep
  endif()
  if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
  endif()
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
  bundled/MurmurHash3.cpp
  src/io.cpp
  src/elias_fano.cpp
  src/kmer_view.cpp
  src/logtools.cpp
)

add_library(biolib ${BIOLIB_SOURCES})

# Tests------------------------------------------------------------------------
if (MAKE_TESTS)
  add_executable(emv_test tests/test_external_memory_vector.cpp)
  target_link_libraries(emv_test argparse)
  add_executable(ous_test tests/test_ordered_unique_sampler.cpp)
  target_link_libraries(ous_test argparse)
  add_executable(kv_test tests/test_kmer_view.cpp)
  target_link_libraries(kv_test argparse z)
  add_executable(j_test tests/test_jaccard.cpp)
  target_link_libraries(j_test argparse z)
  add_executable(rsg_test tests/test_random_sequence_generation.cpp)
  target_link_libraries(rsg_test argparse)

  add_executable(itr_test tests/iterators_test.cpp)
  add_executable(bv_test tests/test_bit_vector.cpp ${BIOLIB_SOURCES})
  add_executable(pv_test tests/test_packed_vector.cpp ${BIOLIB_SOURCES})
  add_executable(rs_test tests/test_rank_select.cpp ${BIOLIB_SOURCES})
  add_executable(ef_test tests/test_elias_fano.cpp ${BIOLIB_SOURCES})
  add_executable(timer_test tests/test_timer.cpp)
  add_executable(popcount_test tests/test_popcount.cpp)
  add_executable(traits_examples tests/traits_examples.cpp bundled/MurmurHash3.cpp)
  add_executable(io_test tests/test_io.cpp ${BIOLIB_SOURCES})
  add_executable(codes_test tests/test_codes.cpp ${BIOLIB_SOURCES})
endif()