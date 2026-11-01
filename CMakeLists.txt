cmake_minimum_required(VERSION 3.16)

project(poivre
   DESCRIPTION "A small, dependency-free C++ unit testing library"
   LANGUAGES CXX
)

# True only when poivre is configured directly, not pulled in via
# add_subdirectory() by a consumer (e.g. chaudiere). Controls whether
# poivre's own self-tests are built by default - a consumer just wants
# the poivre library target, not another test binary in its build.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
   set(POIVRE_IS_TOP_LEVEL TRUE)
else()
   set(POIVRE_IS_TOP_LEVEL FALSE)
endif()

option(POIVRE_BUILD_TESTS "Build poivre's own self-tests" ${POIVRE_IS_TOP_LEVEL})

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

target_compile_features(poivre PUBLIC cxx_std_20)
set_target_properties(poivre PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(poivre PRIVATE
   $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
)

# Headers live at the repo root (no include/ subdirectory), so this is
# the one directory a consumer needs on its include path - handled here
# instead of requiring every consumer to hand-write it.
target_include_directories(poivre
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include/poivre>
)

find_package(Threads REQUIRED)
target_link_libraries(poivre PUBLIC Threads::Threads)

if(POIVRE_BUILD_TESTS)
   enable_testing()

   add_executable(poivre_tests
      tests/SelfTests.cpp
   )

   target_link_libraries(poivre_tests PRIVATE poivre)

   add_test(NAME poivre_tests COMMAND poivre_tests)

   # subprocess regression tests for exit status, exception isolation,
   # zero-test detection and concurrent accounting
   add_subdirectory(tests/regression)
endif()

include(GNUInstallDirs)

install(TARGETS poivre
   EXPORT poivreTargets
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES
   AutoPointer.h
   BasicException.h
   RunCompletionObserver.h
   Runnable.h
   TestCase.h
   TestMacros.h
   TestRegistry.h
   TestSuite.h
   DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/poivre
)
