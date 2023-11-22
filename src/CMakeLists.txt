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
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/bundled>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if (NOT UNNAMED_STANDALONE)
  return()
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_BINARY_DIR}/biolibConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

include(GNUInstallDirs)

install(
  TARGETS biolib EXPORT biolib_Targets
  RUNTIME COMPONENT biolib_Runtime
  LIBRARY COMPONENT biolib_Runtime
  NAMELINK_COMPONENT biolib_Devlopment
  ARCHIVE COMPONENT biolib_Devlopment
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT biolib_Development
)

install(
  EXPORT biolib_Targets
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/biolib"
  NAMESPACE biolib::
  FILE biolib-targets.cmake
  COMPONENT biolib_Development
)

install(
  FILES "${CMAKE_SOURCE_DIR}/cmake/biolibConfig.cmake" "${CMAKE_BINARY_DIR}/biolibConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/biolib"
  COMPONENT biolib_Development
)

file(GLOB_RECURSE BUNDLED_INCLUDE "${CMAKE_SOURCE_DIR}/bundled/*.hpp")

install(
  FILES ${BUNDLED_INCLUDE}
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT biolib_Development
)

configure_file(${CMAKE_SOURCE_DIR}/cmake/biolib.pc.in "${CMAKE_BINARY_DIR}/biolib.pc" @ONLY)
install(
  FILES "${CMAKE_BINARY_DIR}/biolib.pc"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  COMPONENT biolib_Development
)


