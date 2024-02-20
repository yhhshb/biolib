include(CTest)

find_package(ZLIB REQUIRED)

include(FetchContent)
FetchContent_Declare(
  argparse
  GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

macro(add_test_suite name sources)
  add_executable(${name}_test ${CMAKE_SOURCE_DIR}/tests/${sources})
  target_link_libraries(${name}_test PRIVATE build_flags biolib argparse ZLIB::ZLIB)
  add_test(NAME "${name}_test" COMMAND ${name}_test)
endmacro()


add_test_suite(emv test_external_memory_vector.cpp)
add_test_suite(ous test_ordered_unique_sampler.cpp)
add_test_suite(kv test_kmer_view.cpp)
add_test_suite(mmv test_minimizer_view.cpp)
add_test_suite(j test_jaccard.cpp)
add_test_suite(rsg test_random_sequence_generation.cpp)

add_test_suite(itr iterators_test.cpp)
add_test_suite(bv test_bit_vector.cpp)
add_test_suite(pv test_packed_vector.cpp)
add_test_suite(rs test_rank_select.cpp)
add_test_suite(ef test_elias_fano.cpp)
add_test_suite(timer test_timer.cpp)
add_test_suite(popcount test_popcount.cpp)
add_test_suite(traits traits_examples.cpp)
add_test_suite(io test_io.cpp)
add_test_suite(codes test_codes.cpp)
add_test_suite(rlev test_rle_view.cpp)
add_test_suite(bop test_bit_operations.cpp)


