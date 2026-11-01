# Verifies that poivre can be consumed three ways from a standalone project
# (tests/integration/consumer): add_subdirectory(), FetchContent and
# find_package() against an installed package, including after the installed
# prefix has been moved. Uses only CMake, CTest and git, so it runs the same on
# Linux, macOS and Windows.
#
#   cmake -DPOIVRE_SOURCE_DIR=<checkout> -DWORK_DIR=<scratch dir> [-DCONFIG=Debug]
#         [-DCXX_COMPILER=<compiler>] -P tests/integration/run_integration.cmake
#
# WORK_DIR is created from scratch (anything in it is deleted first). The
# FetchContent step clones a throw-away snapshot repository of the tree under
# test, pinned by its exact commit, so uncommitted changes are tested too and
# nothing depends on a moving branch or on the network.

cmake_minimum_required(VERSION 3.16)

foreach(required POIVRE_SOURCE_DIR WORK_DIR)
   if(NOT DEFINED ${required})
      message(FATAL_ERROR "run_integration.cmake: ${required} is required")
   endif()
endforeach()
# optional: use this C++ compiler for every project configured below
set(compilerArgs "")
if(DEFINED CXX_COMPILER AND NOT "${CXX_COMPILER}" STREQUAL "")
   set(compilerArgs "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
endif()

if(NOT DEFINED CONFIG)
   set(CONFIG Debug)
endif()

file(TO_CMAKE_PATH "${POIVRE_SOURCE_DIR}" sourceDir)
file(TO_CMAKE_PATH "${WORK_DIR}" workDir)
set(consumerDir "${sourceDir}/tests/integration/consumer")

if(NOT EXISTS "${consumerDir}/CMakeLists.txt")
   message(FATAL_ERROR "no consumer project under ${consumerDir}")
endif()

file(REMOVE_RECURSE "${workDir}")
file(MAKE_DIRECTORY "${workDir}")

# run a command, show its output only if it fails
function(run description)
   execute_process(COMMAND ${ARGN}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
   if(NOT rc EQUAL 0)
      message("---- ${description}: command failed (${rc}) ----\n${out}${err}\n----")
      message(FATAL_ERROR "FAILED: ${description}")
   endif()
   set(lastOutput "${out}${err}" PARENT_SCOPE)
   message(STATUS "ok: ${description}")
endfunction()

# configure, build and run the consumer; expect exactly ${expectedTests} tests
function(check_consumer name expectedTests)
   set(buildDir "${workDir}/consumer-${name}")
   run("${name}: configure"
      "${CMAKE_COMMAND}" -S "${consumerDir}" -B "${buildDir}"
      -DCMAKE_BUILD_TYPE=${CONFIG} ${compilerArgs} ${ARGN})
   run("${name}: build"
      "${CMAKE_COMMAND}" --build "${buildDir}" --config ${CONFIG})
   run("${name}: ctest"
      "${CMAKE_CTEST_COMMAND}" --test-dir "${buildDir}" -C ${CONFIG}
      --no-tests=error --output-on-failure --verbose)

   if(NOT lastOutput MATCHES "Total Test time")
      message(FATAL_ERROR "${name}: ctest produced no summary")
   endif()
   if(NOT lastOutput MATCHES "consumer: classic=ok registry=ok")
      message("${lastOutput}")
      message(FATAL_ERROR "${name}: the consumer program did not report success")
   endif()
   if(NOT lastOutput MATCHES "100% tests passed, 0 tests failed out of ${expectedTests}")
      message("${lastOutput}")
      message(FATAL_ERROR "${name}: expected ${expectedTests} passing consumer test(s)")
   endif()
   message(STATUS "${name}: ${expectedTests} consumer test(s) passed")
endfunction()

# ---- A. add_subdirectory() ----------------------------------------------------

check_consumer(add_subdirectory 2
   -DPOIVRE_CONSUMER_MODE=add_subdirectory
   "-DPOIVRE_SOURCE_DIR=${sourceDir}")

# ---- B. FetchContent, pinned to an exact revision of a snapshot ---------------

set(snapshotDir "${workDir}/snapshot")
file(MAKE_DIRECTORY "${snapshotDir}")

execute_process(
   COMMAND git ls-files --cached --others --exclude-standard
   WORKING_DIRECTORY "${sourceDir}"
   RESULT_VARIABLE rc OUTPUT_VARIABLE fileList ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
   message(FATAL_ERROR "git ls-files failed in ${sourceDir}: ${err}")
endif()

string(REPLACE "\n" ";" fileList "${fileList}")
foreach(relative IN LISTS fileList)
   string(STRIP "${relative}" relative)
   if(relative STREQUAL "" OR relative MATCHES "^(Testing|\\.claude)/")
      continue()
   endif()
   get_filename_component(relativeDir "${relative}" DIRECTORY)
   file(COPY "${sourceDir}/${relative}" DESTINATION "${snapshotDir}/${relativeDir}")
endforeach()

run("fetchcontent: create snapshot repository"
   git -C "${snapshotDir}" init -q)
run("fetchcontent: stage snapshot"
   git -C "${snapshotDir}" add -A)
run("fetchcontent: commit snapshot"
   git -C "${snapshotDir}" -c user.name=poivre-ci -c user.email=ci@example.invalid
   -c commit.gpgsign=false commit -q -m "snapshot of the tree under test")
run("fetchcontent: read pinned revision"
   git -C "${snapshotDir}" rev-parse HEAD)
string(STRIP "${lastOutput}" pinnedRevision)
message(STATUS "fetchcontent: pinned revision ${pinnedRevision}")

check_consumer(fetchcontent 2
   -DPOIVRE_CONSUMER_MODE=fetchcontent
   "-DPOIVRE_GIT_REPOSITORY=${snapshotDir}"
   "-DPOIVRE_GIT_TAG=${pinnedRevision}")

# ---- C. install, then find_package() ------------------------------------------

set(poivreBuildDir "${workDir}/poivre-build")
set(prefix "${workDir}/prefix")

run("installed: configure poivre"
   "${CMAKE_COMMAND}" -S "${sourceDir}" -B "${poivreBuildDir}"
   -DCMAKE_BUILD_TYPE=${CONFIG} ${compilerArgs} -DPOIVRE_BUILD_TESTS=OFF
   "-DCMAKE_INSTALL_PREFIX=${prefix}")
run("installed: build poivre"
   "${CMAKE_COMMAND}" --build "${poivreBuildDir}" --config ${CONFIG})
run("installed: install poivre into a temporary prefix"
   "${CMAKE_COMMAND}" --install "${poivreBuildDir}" --config ${CONFIG})

foreach(expected
      include/poivre/TestSuite.h include/poivre/TestMacros.h
      include/poivre/TestRegistry.h include/poivre/AutoPointer.h
      lib/cmake/poivre/poivreConfig.cmake
      lib/cmake/poivre/poivreConfigVersion.cmake
      lib/cmake/poivre/poivreTargets.cmake)
   if(NOT EXISTS "${prefix}/${expected}")
      message(FATAL_ERROR "installed package is missing ${expected}")
   endif()
endforeach()

# the historical flat spelling is not part of the installed package
if(EXISTS "${prefix}/include/TestSuite.h" OR EXISTS "${prefix}/include/Runnable.h")
   message(FATAL_ERROR "flat forwarding headers must not be installed")
endif()

# the installed package must be relocatable: no source or build directory in it
file(GLOB_RECURSE installedCmakeFiles "${prefix}/lib/cmake/*.cmake")
foreach(file IN LISTS installedCmakeFiles)
   file(READ "${file}" text)
   foreach(forbidden "${sourceDir}" "${poivreBuildDir}" "${workDir}")
      string(FIND "${text}" "${forbidden}" where)
      if(NOT where EQUAL -1)
         message(FATAL_ERROR "${file} contains the absolute path ${forbidden}")
      endif()
   endforeach()
endforeach()
message(STATUS "ok: installed package files contain no source/build paths")

check_consumer(installed 1
   -DPOIVRE_CONSUMER_MODE=installed
   "-DCMAKE_PREFIX_PATH=${prefix}"
   "-DPOIVRE_FORBIDDEN_DIRS=${sourceDir}|${poivreBuildDir}"
   "-DPOIVRE_EXPECT_PREFIX=${prefix}")

# ---- D. relocation: move the prefix, remove everything it came from -----------

set(relocatedPrefix "${workDir}/relocated prefix")
file(RENAME "${prefix}" "${relocatedPrefix}")
file(REMOVE_RECURSE "${poivreBuildDir}")
if(EXISTS "${prefix}")
   message(FATAL_ERROR "the original prefix still exists after the move")
endif()

check_consumer(relocated 1
   -DPOIVRE_CONSUMER_MODE=installed
   "-DCMAKE_PREFIX_PATH=${relocatedPrefix}"
   "-DPOIVRE_FORBIDDEN_DIRS=${sourceDir}|${poivreBuildDir}|${prefix}"
   "-DPOIVRE_EXPECT_PREFIX=${relocatedPrefix}")

# ---- E. a target named "poivre" that already exists -----------------------------
#
# The same poivre included again, or a compatible poivre from another directory,
# is accepted (first definition wins, `poivre::poivre` still resolves). An
# unrelated target called "poivre", a poivre of another major version, and an
# imported `poivre::poivre` mixed with the source tree are rejected with an
# explanation.

# a copy of the tree under test that claims to be poivre 9.0.0
set(modifiedDir "${workDir}/poivre-9")
file(COPY "${snapshotDir}/" DESTINATION "${modifiedDir}" PATTERN ".git" EXCLUDE)
file(READ "${modifiedDir}/CMakeLists.txt" modifiedText)
string(REPLACE "VERSION 0.1.0" "VERSION 9.0.0" modifiedTextNew "${modifiedText}")
if(modifiedTextNew STREQUAL modifiedText)
   message(FATAL_ERROR "could not make the version-9 copy of poivre")
endif()
file(WRITE "${modifiedDir}/CMakeLists.txt" "${modifiedTextNew}")

# check_duplicate(<name> ACCEPT|REJECT <text expected literally in cmake's output>
#                 BODY <CMake code, with @SRC@ @SNAP@ @MODIFIED@ for paths>
#                 [ARGS <extra configure arguments>...])
function(check_duplicate name mode)
   cmake_parse_arguments(D "" "EXPECT;BODY" "ARGS" ${ARGN})
   set(projectDir "${workDir}/dup-${name}")
   file(MAKE_DIRECTORY "${projectDir}")
   file(WRITE "${projectDir}/unrelated.cpp" "int unrelated() { return 0; }\n")
   file(WRITE "${projectDir}/main.cpp"
      "#include <poivre/TestSuite.h>\nint main() { return 0; }\n")

   set(body "${D_BODY}")
   string(REPLACE "@SRC@" "${sourceDir}" body "${body}")
   string(REPLACE "@SNAP@" "${snapshotDir}" body "${body}")
   string(REPLACE "@MODIFIED@" "${modifiedDir}" body "${body}")
   file(WRITE "${projectDir}/CMakeLists.txt"
      "cmake_minimum_required(VERSION 3.16)\nproject(dup CXX)\n${body}\n")

   execute_process(
      COMMAND "${CMAKE_COMMAND}" -S "${projectDir}" -B "${projectDir}/build"
         -DCMAKE_BUILD_TYPE=${CONFIG} ${compilerArgs} ${D_ARGS}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
   set(text "${out}${err}")
   # CMake word-wraps error text; compare with runs of whitespace collapsed
   string(REGEX REPLACE "[ \t\r\n]+" " " normalized "${text}")
   string(FIND "${normalized}" "${D_EXPECT}" where)

   if(mode STREQUAL "ACCEPT")
      if(NOT rc EQUAL 0 OR where EQUAL -1)
         message("${text}")
         message(FATAL_ERROR "duplicate ${name}: expected the configure to succeed and mention '${D_EXPECT}'")
      endif()
      run("duplicate ${name}: build"
         "${CMAKE_COMMAND}" --build "${projectDir}/build" --config ${CONFIG})
   else()
      if(rc EQUAL 0 OR where EQUAL -1)
         message("${text}")
         message(FATAL_ERROR "duplicate ${name}: expected the configure to fail and mention '${D_EXPECT}'")
      endif()
   endif()
   message(STATUS "ok: duplicate ${name}: ${mode} as expected")
endfunction()

check_duplicate(same_source_twice ACCEPT
   EXPECT "already defined in ${sourceDir}"
   BODY [=[
add_subdirectory("@SRC@" poivre_a)
add_subdirectory("@SRC@" poivre_b)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE poivre::poivre)
]=])

check_duplicate(compatible_copy_elsewhere ACCEPT
   EXPECT "already defined in ${sourceDir}"
   BODY [=[
add_subdirectory("@SRC@" poivre_a)
add_subdirectory("@SNAP@" poivre_b)
add_executable(app main.cpp)
target_link_libraries(app PRIVATE poivre::poivre)
]=])

check_duplicate(unrelated_target REJECT
   EXPECT "is not a poivre library"
   BODY [=[
add_library(poivre STATIC unrelated.cpp)
add_subdirectory("@SRC@" poivre_a)
]=])

check_duplicate(incompatible_major_version REJECT
   EXPECT "is not compatible with version 9.0.0"
   BODY [=[
add_subdirectory("@SRC@" poivre_a)
add_subdirectory("@MODIFIED@" poivre_b)
]=])

check_duplicate(imported_and_source REJECT
   EXPECT "already exists (for example from find_package(poivre))"
   ARGS "-DCMAKE_PREFIX_PATH=${relocatedPrefix}"
   BODY [=[
find_package(poivre CONFIG REQUIRED)
add_subdirectory("@SRC@" poivre_a)
]=])

message(STATUS "poivre integration: add_subdirectory, FetchContent, find_package, relocation and duplicate-target handling all verified")
