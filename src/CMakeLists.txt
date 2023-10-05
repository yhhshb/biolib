file(GLOB_RECURSE BIOLIB_SOURCES "*.cpp")

add_library(biolib ${BIOLIB_SOURCES} ${BUNDLED_SOURCES})
add_library(biolib::biolib ALIAS biolib)

set_target_properties(biolib PROPERTIES
  VERSION ${CMAKE_PROJECT_VERSION}
  SOVERSION ${CMAKE_PROJECT_VERSION_MAJOR}
)

target_compile_features(biolib PUBLIC cxx_std_17)

target_link_libraries(biolib PRIVATE
  $<BUILD_INTERFACE:build_flags>
  $<BUILD_INTERFACE:warning_flags>
  $<BUILD_INTERFACE:asan_flags>
)

target_include_directories(biolib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/bundled>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_BINARY_DIR}/biolibConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)
