# A minimal, standalone consumer of poivre. It is NOT part of poivre's own
# build; tests/integration/run_integration.cmake configures it three ways:
#
#   POIVRE_CONSUMER_MODE=add_subdirectory  poivre from a source checkout
#   POIVRE_CONSUMER_MODE=fetchcontent      poivre acquired with FetchContent
#   POIVRE_CONSUMER_MODE=installed         poivre from an installed package
#
# In every mode it links poivre::poivre and nothing else of poivre's.

cmake_minimum_required(VERSION 3.16)
project(poivre_consumer LANGUAGES CXX)

set(POIVRE_CONSUMER_MODE "installed" CACHE STRING
    "add_subdirectory | fetchcontent | installed")
set(POIVRE_SOURCE_DIR "" CACHE PATH "poivre source checkout (add_subdirectory mode)")
set(POIVRE_GIT_REPOSITORY "" CACHE STRING "repository to fetch (fetchcontent mode)")
set(POIVRE_GIT_TAG "" CACHE STRING "exact revision to fetch (fetchcontent mode)")
set(POIVRE_FORBIDDEN_DIRS "" CACHE STRING
    "installed mode: directories (separated by |) that must not appear in poivre::poivre properties")
set(POIVRE_EXPECT_PREFIX "" CACHE PATH
    "installed mode: prefix that poivre::poivre's headers and library must live under")

if(POIVRE_CONSUMER_MODE STREQUAL "add_subdirectory")
   add_subdirectory("${POIVRE_SOURCE_DIR}" poivre-build)
elseif(POIVRE_CONSUMER_MODE STREQUAL "fetchcontent")
   include(FetchContent)
   FetchContent_Declare(poivre
      GIT_REPOSITORY "${POIVRE_GIT_REPOSITORY}"
      GIT_TAG        "${POIVRE_GIT_TAG}"
   )
   FetchContent_MakeAvailable(poivre)
elseif(POIVRE_CONSUMER_MODE STREQUAL "installed")
   find_package(poivre CONFIG REQUIRED)
   message(STATUS "poivre ${poivre_VERSION} found at ${poivre_DIR}")
else()
   message(FATAL_ERROR "unknown POIVRE_CONSUMER_MODE '${POIVRE_CONSUMER_MODE}'")
endif()

# ---- the imported package must not point back into the source or build tree

if(POIVRE_CONSUMER_MODE STREQUAL "installed")
   get_target_property(includeDirs poivre::poivre INTERFACE_INCLUDE_DIRECTORIES)
   get_target_property(libraryFile poivre::poivre IMPORTED_LOCATION_DEBUG)
   if(NOT libraryFile)
      get_target_property(libraryFile poivre::poivre IMPORTED_LOCATION_RELEASE)
   endif()
   if(NOT libraryFile)
      get_target_property(libraryFile poivre::poivre IMPORTED_LOCATION)
   endif()
   message(STATUS "poivre::poivre include directories: ${includeDirs}")
   message(STATUS "poivre::poivre library: ${libraryFile}")

   string(REPLACE "|" ";" forbiddenDirs "${POIVRE_FORBIDDEN_DIRS}")
   foreach(forbidden IN LISTS forbiddenDirs)
      foreach(property includeDirs libraryFile)
         string(FIND "${${property}}" "${forbidden}" where)
         if(NOT where EQUAL -1)
            message(FATAL_ERROR
               "poivre::poivre ${property} refers to '${forbidden}': ${${property}}")
         endif()
      endforeach()
   endforeach()

   if(POIVRE_EXPECT_PREFIX)
      foreach(dir IN LISTS includeDirs)
         string(FIND "${dir}" "${POIVRE_EXPECT_PREFIX}" where)
         if(NOT where EQUAL 0)
            message(FATAL_ERROR
               "include directory '${dir}' is not under the expected prefix '${POIVRE_EXPECT_PREFIX}'")
         endif()
      endforeach()
      string(FIND "${libraryFile}" "${POIVRE_EXPECT_PREFIX}" where)
      if(NOT where EQUAL 0)
         message(FATAL_ERROR
            "library '${libraryFile}' is not under the expected prefix '${POIVRE_EXPECT_PREFIX}'")
      endif()
   endif()
endif()

enable_testing()

# <poivre/...> include style (the only one an installed package provides)
add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE poivre::poivre)
add_test(NAME consumer COMMAND consumer)

# historical flat style ("TestSuite.h"), available when poivre's source tree is
# on the include path, i.e. in the add_subdirectory and fetchcontent modes
if(NOT POIVRE_CONSUMER_MODE STREQUAL "installed")
   add_executable(consumer_flat main_flat.cpp)
   target_link_libraries(consumer_flat PRIVATE poivre::poivre)
   add_test(NAME consumer_flat COMMAND consumer_flat)
endif()
