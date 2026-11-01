cmake_minimum_required(VERSION 3.16)

project(poivre
   VERSION 0.1.0
   DESCRIPTION "A small, dependency-free C++ unit testing library"
   LANGUAGES CXX
)

# A consumer that vendors poivre more than once (for example once itself and
# once inside a submodule such as chaudiere) would otherwise fail with
# "add_library cannot create target poivre ... already exists". The first
# definition wins, but only if the existing target really is a compatible
# poivre library; an unrelated target that merely has the name "poivre", or a
# poivre of another major version, is an error with an explanation.
if(TARGET poivre)
   get_target_property(poivreExistingVersion poivre POIVRE_TARGET_VERSION)
   get_target_property(poivreExistingType poivre TYPE)
   get_target_property(poivreExistingDir poivre SOURCE_DIR)

   if(NOT poivreExistingVersion)
      message(FATAL_ERROR
         "poivre: a CMake target named 'poivre' already exists, but it is not a "
         "poivre library (type: ${poivreExistingType}, defined in: "
         "${poivreExistingDir}). poivre (${CMAKE_CURRENT_SOURCE_DIR}) needs that "
         "name; rename the other target. If it is an older copy of poivre, add "
         "the newer copy first or update it.")
   endif()

   string(REGEX MATCH "^[0-9]+" poivreExistingMajor "${poivreExistingVersion}")
   if(NOT poivreExistingMajor STREQUAL PROJECT_VERSION_MAJOR)
      message(FATAL_ERROR
         "poivre: version ${poivreExistingVersion} is already defined in "
         "${poivreExistingDir}, which is not compatible with version "
         "${PROJECT_VERSION} in ${CMAKE_CURRENT_SOURCE_DIR}. Use one major "
         "version of poivre per build.")
   endif()

   if(TARGET poivre::poivre)
      get_target_property(poivreExistingAlias poivre::poivre ALIASED_TARGET)
      if(NOT poivreExistingAlias STREQUAL "poivre")
         message(FATAL_ERROR
            "poivre: 'poivre::poivre' exists but is not an alias of the poivre "
            "target defined in ${poivreExistingDir}.")
      endif()
   else()
      add_library(poivre::poivre ALIAS poivre)
   endif()

   message(STATUS
      "poivre: version ${poivreExistingVersion} already defined in "
      "${poivreExistingDir}; using it instead of ${CMAKE_CURRENT_SOURCE_DIR}")
   return()
endif()

if(TARGET poivre::poivre)
   message(FATAL_ERROR
      "poivre: the target 'poivre::poivre' already exists (for example from "
      "find_package(poivre)) but there is no 'poivre' target. Use either the "
      "installed package or the poivre source tree in one build, not both.")
endif()

# True only when poivre is configured directly, not pulled in via
# add_subdirectory() or FetchContent by a consumer (e.g. chaudiere). Controls
# whether poivre's own self-tests are built, and whether it installs itself:
# a consumer just wants the poivre library target, not another test binary
# or poivre's files in its own install tree.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
   set(POIVRE_IS_TOP_LEVEL TRUE)
else()
   set(POIVRE_IS_TOP_LEVEL FALSE)
endif()

option(POIVRE_BUILD_TESTS "Build poivre's own self-tests" ${POIVRE_IS_TOP_LEVEL})
option(POIVRE_INSTALL "Generate poivre's install and package-config rules" ${POIVRE_IS_TOP_LEVEL})

include(GNUInstallDirs)

# Static by default (respects BUILD_SHARED_LIBS if a consumer sets it).
# Every current consumer of poivre (chaudiere, misere, tonnerre, chapeau)
# already compiles these three files directly into its own test binary
# rather than linking a shared poivre.so at runtime, so static is both
# the safer default and the one that matches how poivre is actually used
# today.
add_library(poivre
   TestCase.cpp
   TestSuite.cpp
   TestRegistry.cpp
)

# The target keeps its historical name "poivre"; "poivre::poivre" is the same
# target and is what new code should link, whether poivre was added with
# add_subdirectory(), FetchContent or find_package().
add_library(poivre::poivre ALIAS poivre)
# POIVRE_TARGET_VERSION marks this as a genuine poivre target (see the
# duplicate-target check at the top of this file)
set_target_properties(poivre PROPERTIES
   EXPORT_NAME poivre
   POIVRE_TARGET_VERSION "${PROJECT_VERSION}"
)

target_compile_features(poivre PUBLIC cxx_std_20)
set_target_properties(poivre PROPERTIES CXX_EXTENSIONS OFF)

# MSVC is also true for clang-cl, where -Wall would mean -Weverything
if(MSVC)
   target_compile_options(poivre PRIVATE /W4)
else()
   target_compile_options(poivre PRIVATE
      $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
   )
endif()

# Public headers live in include/poivre/ and are included as
# <poivre/TestSuite.h>. In a source tree the repository root is on the include
# path as well, which keeps the historical flat spelling ("TestSuite.h", served
# by the forwarding headers at the root) working for existing consumers. The
# installed package provides only the <poivre/...> spelling.
target_include_directories(poivre
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# poivre itself uses no threads (only std::atomic, <filesystem>, <random>), so it
# does not link Threads::Threads. Code that starts threads links it itself.

if(POIVRE_BUILD_TESTS)
   enable_testing()

   find_package(Threads REQUIRED)

   add_executable(poivre_tests
      tests/SelfTests.cpp
   )

   # the self-tests start threads of their own
   target_link_libraries(poivre_tests PRIVATE poivre Threads::Threads)

   add_test(NAME poivre_tests COMMAND poivre_tests)

   # subprocess regression tests for exit status, exception isolation,
   # zero-test detection and concurrent accounting
   add_subdirectory(tests/regression)
endif()

if(POIVRE_INSTALL)
   include(CMakePackageConfigHelpers)

   set(POIVRE_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/poivre)

   install(TARGETS poivre
      EXPORT poivreTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   )

   install(DIRECTORY include/poivre
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      FILES_MATCHING PATTERN "*.h"
   )

   install(EXPORT poivreTargets
      NAMESPACE poivre::
      DESTINATION ${POIVRE_INSTALL_CMAKEDIR}
   )

   configure_package_config_file(
      cmake/poivreConfig.cmake.in
      ${CMAKE_CURRENT_BINARY_DIR}/poivreConfig.cmake
      INSTALL_DESTINATION ${POIVRE_INSTALL_CMAKEDIR}
   )

   write_basic_package_version_file(
      ${CMAKE_CURRENT_BINARY_DIR}/poivreConfigVersion.cmake
      VERSION ${PROJECT_VERSION}
      COMPATIBILITY SameMajorVersion
   )

   install(FILES
      ${CMAKE_CURRENT_BINARY_DIR}/poivreConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/poivreConfigVersion.cmake
      DESTINATION ${POIVRE_INSTALL_CMAKEDIR}
   )
endif()
