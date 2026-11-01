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
#
# Stages: A add_subdirectory, B FetchContent, C installed package, D relocated
# package, E duplicate-target handling.
#
# Failure reporting: the real work runs in a child `cmake -P` (the same file,
# with INTEGRATION_INNER set) so that even an error in the script itself is
# caught and reported. When GITHUB_ACTIONS=true a failure is also written as
# `::error::` workflow annotations (stage, command, exit code, stdout, stderr),
# which can be read through the public GitHub API without signing in. The
# environment and any credentials are never printed.

cmake_minimum_required(VERSION 3.16)

# ---- reporting helpers ----------------------------------------------------------

# annotation text is limited, so only the last part of long output is kept
set(maxAnnotationChars 6000)

# replace credentials by *** (the usual token variables and GitHub token shapes)
function(mask_secrets variable)
   set(text "${${variable}}")
   foreach(name GITHUB_TOKEN ACTIONS_RUNTIME_TOKEN ACTIONS_ID_TOKEN_REQUEST_TOKEN
                SYSTEM_ACCESSTOKEN)
      if(NOT "$ENV{${name}}" STREQUAL "")
         string(REPLACE "$ENV{${name}}" "***" text "${text}")
      endif()
   endforeach()
   string(REGEX REPLACE "gh[pousr]_[A-Za-z0-9_]+" "***" text "${text}")
   set(${variable} "${text}" PARENT_SCOPE)
endfunction()

# keep only the last `limit` characters of a long text
function(keep_tail variable limit)
   set(text "${${variable}}")
   string(LENGTH "${text}" length)
   if(length GREATER limit)
      math(EXPR start "${length} - ${limit}")
      string(SUBSTRING "${text}" ${start} ${limit} text)
      set(text "[... first ${start} of ${length} characters omitted ...]\n${text}")
   endif()
   set(${variable} "${text}" PARENT_SCOPE)
endfunction()

# GitHub Actions workflow-command escaping: % first, then CR and LF; property
# values (the title) additionally escape : and ,
function(escape_workflow_data variable)
   set(text "${${variable}}")
   string(REPLACE "%" "%25" text "${text}")
   string(REPLACE "\r" "%0D" text "${text}")
   string(REPLACE "\n" "%0A" text "${text}")
   set(${variable} "${text}" PARENT_SCOPE)
endfunction()

function(escape_workflow_property variable)
   escape_workflow_data(${variable})
   set(text "${${variable}}")
   string(REPLACE ":" "%3A" text "${text}")
   string(REPLACE "," "%2C" text "${text}")
   set(${variable} "${text}" PARENT_SCOPE)
endfunction()

# annotate(<error|warning|notice> <title> <text>)
function(annotate level title text)
   mask_secrets(text)
   keep_tail(text ${maxAnnotationChars})
   if("$ENV{GITHUB_ACTIONS}" STREQUAL "true")
      escape_workflow_data(text)
      escape_workflow_property(title)
      # `cmake -E echo` writes the command on one line of stdout
      execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "::${level} title=${title}::${text}")
   elseif(NOT level STREQUAL "notice")
      # outside GitHub Actions plain messages already carry the same
      # information, and notices only repeat what the entry point printed
      message("[${level}] ${title}\n${text}")
   endif()
endfunction()

# integration_fail(<message> [COMMAND <command line>] [RC <exit code>]
#                  [STDOUT <text>] [STDERR <text>])
# Reports stage, command, exit code and output, then stops with an error.
function(integration_fail message)
   cmake_parse_arguments(F "" "COMMAND;RC;STDOUT;STDERR" "" ${ARGN})
   set(header "stage ${integrationStage}: ${message}")
   if(F_COMMAND)
      string(APPEND header "\ncommand: ${F_COMMAND}")
   endif()
   if(DEFINED F_RC)
      string(APPEND header "\nexit code: ${F_RC}")
   endif()
   annotate(error "Package consumers: stage ${integrationStage}" "${header}")
   if(NOT "${F_STDOUT}" STREQUAL "")
      annotate(error "stdout (stage ${integrationStage})" "${F_STDOUT}")
   endif()
   if(NOT "${F_STDERR}" STREQUAL "")
      annotate(error "stderr (stage ${integrationStage})" "${F_STDERR}")
   endif()
   message(FATAL_ERROR "${message}")
endfunction()

# ---- entry point: report the environment, run the stages in a child cmake -------

if(NOT INTEGRATION_INNER)
   foreach(required POIVRE_SOURCE_DIR WORK_DIR)
      if(NOT DEFINED ${required})
         message(FATAL_ERROR "run_integration.cmake: ${required} is required")
      endif()
   endforeach()

   if(NOT DEFINED CONFIG)
      set(CONFIG Debug)
   endif()

   cmake_host_system_information(RESULT osName QUERY OS_NAME)
   cmake_host_system_information(RESULT osRelease QUERY OS_RELEASE)
   cmake_host_system_information(RESULT osPlatform QUERY OS_PLATFORM)
   execute_process(COMMAND git --version
      OUTPUT_VARIABLE gitVersion ERROR_VARIABLE gitError RESULT_VARIABLE gitResult)
   if(gitResult EQUAL 0)
      string(STRIP "${gitVersion}" gitVersion)
   else()
      set(gitVersion "git not available (${gitResult})")
   endif()

   set(compilerText "")
   if(DEFINED CXX_COMPILER AND NOT "${CXX_COMPILER}" STREQUAL "")
      set(compilerText "${CXX_COMPILER} (selected with -DCXX_COMPILER)")
   else()
      set(compilerText "CMake's default")
   endif()
   foreach(name CXX CC)
      if(NOT "$ENV{${name}}" STREQUAL "")
         string(APPEND compilerText "; environment ${name}=$ENV{${name}}")
      endif()
   endforeach()

   if(NOT "$ENV{CMAKE_GENERATOR}" STREQUAL "")
      set(generatorText "$ENV{CMAKE_GENERATOR} (CMAKE_GENERATOR)")
   else()
      set(generatorText "not explicitly selected (CMake's default for this platform)")
   endif()

   set(environmentText "")
   string(APPEND environmentText "operating system: ${osName} ${osRelease} (${osPlatform})\n")
   string(APPEND environmentText "CMake: ${CMAKE_VERSION}\n")
   string(APPEND environmentText "Git: ${gitVersion}\n")
   string(APPEND environmentText "compiler: ${compilerText}\n")
   string(APPEND environmentText "generator: ${generatorText}\n")
   string(APPEND environmentText "configuration: ${CONFIG}\n")
   string(APPEND environmentText "working directory: ${CMAKE_BINARY_DIR}\n")
   string(APPEND environmentText "source directory: ${POIVRE_SOURCE_DIR}\n")
   string(APPEND environmentText "work directory: ${WORK_DIR}")
   message("poivre integration environment:\n${environmentText}")
   annotate(notice "Package consumers: environment" "${environmentText}")

   set(innerArguments -DINTEGRATION_INNER=ON
      "-DPOIVRE_SOURCE_DIR=${POIVRE_SOURCE_DIR}" "-DWORK_DIR=${WORK_DIR}")
   foreach(name CONFIG CXX_COMPILER)
      if(DEFINED ${name})
         list(APPEND innerArguments "-D${name}=${${name}}")
      endif()
   endforeach()

   set(logOut "${WORK_DIR}-integration.stdout")
   set(logErr "${WORK_DIR}-integration.stderr")
   file(REMOVE "${logOut}" "${logErr}")

   execute_process(
      COMMAND "${CMAKE_COMMAND}" ${innerArguments} -P "${CMAKE_CURRENT_LIST_FILE}"
      RESULT_VARIABLE innerResult
      OUTPUT_FILE "${logOut}"
      ERROR_FILE "${logErr}"
      TIMEOUT 1800)

   # show what the child printed (this also emits any annotations it wrote)
   execute_process(COMMAND "${CMAKE_COMMAND}" -E cat "${logOut}")
   execute_process(COMMAND "${CMAKE_COMMAND}" -E cat "${logErr}")

   if(NOT innerResult EQUAL 0)
      file(READ "${logOut}" innerStdout)
      file(READ "${logErr}" innerStderr)
      # a failure the child reported itself already carries stage, command,
      # exit code and output; anything else (an error in the script, a crash, a
      # timeout) is reported here with the last stage the child announced
      string(FIND "${innerStdout}" "::error" alreadyReported)
      if(alreadyReported EQUAL -1)
         set(lastStage "unknown")
         string(REGEX MATCHALL "-- stage: [^\n]*" stageLines "${innerStdout}")
         if(stageLines)
            list(GET stageLines -1 lastStage)
            string(REPLACE "-- stage: " "" lastStage "${lastStage}")
         endif()
         annotate(error "Package consumers: script failed"
            "stage ${lastStage}\nexit code: ${innerResult}\n(no stage report was produced; the child cmake stopped on its own error)")
         annotate(error "stderr (script failure)" "${innerStderr}")
         annotate(error "stdout (script failure)" "${innerStdout}")
      endif()
   endif()

   file(REMOVE "${logOut}" "${logErr}")
   if(NOT innerResult EQUAL 0)
      message(FATAL_ERROR "poivre integration failed (child cmake exit code: ${innerResult})")
   endif()
   return()
endif()

# ---- from here on: the stages (run by the child cmake) ---------------------------

# announce a stage; the entry point uses the last announcement if the child dies
macro(begin_stage name)
   set(integrationStage "${name}")
   message(STATUS "stage: ${name}")
endmacro()

begin_stage("setup")

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
   integration_fail("no consumer project under ${consumerDir}")
endif()

file(REMOVE_RECURSE "${workDir}")
file(MAKE_DIRECTORY "${workDir}")

# run a command, show its output only if it fails
function(run description)
   execute_process(COMMAND ${ARGN}
      RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
   if(NOT rc EQUAL 0)
      list(JOIN ARGN " " commandLine)
      message("---- ${description}: command failed (${rc}) ----\n${out}${err}\n----")
      integration_fail("FAILED: ${description}"
         COMMAND "${commandLine}" RC "${rc}" STDOUT "${out}" STDERR "${err}")
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
      integration_fail("${name}: ctest produced no summary" STDOUT "${lastOutput}")
   endif()
   if(NOT lastOutput MATCHES "consumer: classic=ok registry=ok")
      message("${lastOutput}")
      integration_fail("${name}: the consumer program did not report success"
         STDOUT "${lastOutput}")
   endif()
   if(NOT lastOutput MATCHES "100% tests passed, 0 tests failed out of ${expectedTests}")
      message("${lastOutput}")
      integration_fail("${name}: expected ${expectedTests} passing consumer test(s)"
         STDOUT "${lastOutput}")
   endif()
   message(STATUS "${name}: ${expectedTests} consumer test(s) passed")
endfunction()

# ---- A. add_subdirectory() ----------------------------------------------------

begin_stage("A (add_subdirectory)")

check_consumer(add_subdirectory 2
   -DPOIVRE_CONSUMER_MODE=add_subdirectory
   "-DPOIVRE_SOURCE_DIR=${sourceDir}")

# ---- B. FetchContent, pinned to an exact revision of a snapshot ---------------

begin_stage("B (FetchContent)")

set(snapshotDir "${workDir}/snapshot")
file(MAKE_DIRECTORY "${snapshotDir}")

execute_process(
   COMMAND git ls-files --cached --others --exclude-standard
   WORKING_DIRECTORY "${sourceDir}"
   RESULT_VARIABLE rc OUTPUT_VARIABLE fileList ERROR_VARIABLE err)
if(NOT rc EQUAL 0)
   integration_fail("git ls-files failed in ${sourceDir}"
      COMMAND "git ls-files --cached --others --exclude-standard" RC "${rc}"
      STDOUT "${fileList}" STDERR "${err}")
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

begin_stage("C (installed package)")

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
      integration_fail("installed package is missing ${expected}")
   endif()
endforeach()

# the historical flat spelling is not part of the installed package
if(EXISTS "${prefix}/include/TestSuite.h" OR EXISTS "${prefix}/include/Runnable.h")
   integration_fail("flat forwarding headers must not be installed")
endif()

# the installed package must be relocatable: no source or build directory in it
file(GLOB_RECURSE installedCmakeFiles "${prefix}/lib/cmake/*.cmake")
foreach(file IN LISTS installedCmakeFiles)
   file(READ "${file}" text)
   foreach(forbidden "${sourceDir}" "${poivreBuildDir}" "${workDir}")
      string(FIND "${text}" "${forbidden}" where)
      if(NOT where EQUAL -1)
         integration_fail("${file} contains the absolute path ${forbidden}")
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

begin_stage("D (relocated package)")

set(relocatedPrefix "${workDir}/relocated prefix")
file(RENAME "${prefix}" "${relocatedPrefix}")
file(REMOVE_RECURSE "${poivreBuildDir}")
if(EXISTS "${prefix}")
   integration_fail("the original prefix still exists after the move")
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

begin_stage("E (duplicate-target handling)")

# a copy of the tree under test that claims to be poivre 9.0.0
set(modifiedDir "${workDir}/poivre-9")
file(COPY "${snapshotDir}/" DESTINATION "${modifiedDir}" PATTERN ".git" EXCLUDE)
file(READ "${modifiedDir}/CMakeLists.txt" modifiedText)
string(REPLACE "VERSION 0.1.0" "VERSION 9.0.0" modifiedTextNew "${modifiedText}")
if(modifiedTextNew STREQUAL modifiedText)
   integration_fail("could not make the version-9 copy of poivre")
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
   set(configureCommand "${CMAKE_COMMAND} -S ${projectDir} -B ${projectDir}/build -DCMAKE_BUILD_TYPE=${CONFIG} ${compilerArgs} ${D_ARGS}")
   # CMake word-wraps error text; compare with runs of whitespace collapsed
   string(REGEX REPLACE "[ \t\r\n]+" " " normalized "${text}")
   string(FIND "${normalized}" "${D_EXPECT}" where)

   if(mode STREQUAL "ACCEPT")
      if(NOT rc EQUAL 0 OR where EQUAL -1)
         message("${text}")
         integration_fail("duplicate ${name}: expected the configure to succeed and mention '${D_EXPECT}'"
            COMMAND "${configureCommand}" RC "${rc}" STDOUT "${out}" STDERR "${err}")
      endif()
      run("duplicate ${name}: build"
         "${CMAKE_COMMAND}" --build "${projectDir}/build" --config ${CONFIG})
   else()
      if(rc EQUAL 0 OR where EQUAL -1)
         message("${text}")
         integration_fail("duplicate ${name}: expected the configure to fail and mention '${D_EXPECT}'"
            COMMAND "${configureCommand}" RC "${rc}" STDOUT "${out}" STDERR "${err}")
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
