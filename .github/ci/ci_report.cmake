# Reports (and checks) what a CI job configured, built and discovered, using
# only CMake/CTest so it behaves the same on Linux, macOS and Windows.
#
#   cmake -DBUILD_DIR=build -DCONFIG=Debug -DMIN_TESTS=17 -P .github/ci/ci_report.cmake
#
# Environment (all optional): CI_PLATFORM (a label for the job),
# CI_BUILD_RESULT and CI_TEST_RESULT (the outcomes of the earlier steps).
#
# Writes a short table to the log and, when GITHUB_STEP_SUMMARY is set, to the
# job summary. Fails if fewer than MIN_TESTS tests are discovered.

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED BUILD_DIR)
   set(BUILD_DIR build)
endif()
if(NOT DEFINED CONFIG)
   set(CONFIG Debug)
endif()
if(NOT DEFINED MIN_TESTS)
   set(MIN_TESTS 1)
endif()

# compiler identity, recorded by CMake when it configured the build tree
set(compilerId "unknown")
set(compilerVersion "unknown")
set(compilerPath "unknown")
file(GLOB compilerInfoFiles "${BUILD_DIR}/CMakeFiles/*/CMakeCXXCompiler.cmake")
if(compilerInfoFiles)
   list(GET compilerInfoFiles 0 compilerInfoFile)
   include("${compilerInfoFile}")
   set(compilerId "${CMAKE_CXX_COMPILER_ID}")
   set(compilerVersion "${CMAKE_CXX_COMPILER_VERSION}")
   set(compilerPath "${CMAKE_CXX_COMPILER}")
   if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT AND NOT "${compilerId}" STREQUAL "MSVC")
      set(compilerId "${compilerId} (${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}-style front end)")
   endif()
endif()

# tests discovered by CTest
set(testsDiscovered 0)
execute_process(
   COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BUILD_DIR}" -C "${CONFIG}" -N
   OUTPUT_VARIABLE ctestList
   RESULT_VARIABLE ctestListResult
)
if(ctestList MATCHES "Total Tests: ([0-9]+)")
   set(testsDiscovered "${CMAKE_MATCH_1}")
endif()

foreach(name PLATFORM BUILD_RESULT TEST_RESULT INTEGRATION_RESULT)
   set(value "$ENV{CI_${name}}")
   if("${value}" STREQUAL "")
      set(value "not reported")
   endif()
   set(ci_${name} "${value}")
endforeach()

cmake_host_system_information(RESULT hostOsName QUERY OS_NAME)
cmake_host_system_information(RESULT hostOsRelease QUERY OS_RELEASE)
cmake_host_system_information(RESULT hostOsPlatform QUERY OS_PLATFORM)
set(platform "${hostOsName} ${hostOsRelease} (${hostOsPlatform})")

set(table "")
string(APPEND table "### ${ci_PLATFORM}\n\n")
string(APPEND table "| Item | Value |\n|---|---|\n")
string(APPEND table "| Platform | ${platform} |\n")
string(APPEND table "| Compiler | ${compilerId} |\n")
string(APPEND table "| Compiler version | ${compilerVersion} |\n")
string(APPEND table "| Compiler path | `${compilerPath}` |\n")
string(APPEND table "| Tests discovered | ${testsDiscovered} (required: at least ${MIN_TESTS}) |\n")
string(APPEND table "| Build result | ${ci_BUILD_RESULT} |\n")
string(APPEND table "| Test result | ${ci_TEST_RESULT} |\n")
string(APPEND table "| Package consumers (add_subdirectory, FetchContent, find_package) | ${ci_INTEGRATION_RESULT} |\n")

message("${table}")

if(NOT "$ENV{GITHUB_STEP_SUMMARY}" STREQUAL "")
   file(APPEND "$ENV{GITHUB_STEP_SUMMARY}" "${table}\n")
endif()

if(testsDiscovered LESS MIN_TESTS)
   message(FATAL_ERROR
      "CTest discovered ${testsDiscovered} test(s) in '${BUILD_DIR}'; at least ${MIN_TESTS} are required")
endif()
