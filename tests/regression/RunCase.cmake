# Runs one regression helper executable as a subprocess and checks that it
#   - exited with exactly EXPECT_EXIT (a crash/abort/timeout never matches,
#     because execute_process then reports a message instead of a number),
#   - printed every MATCH_<n> regular expression,
#   - printed no REJECT_<n> regular expression,
#   - and, if CONCURRENT_LINES is set, printed exactly that many failure
#     lines, each one complete and uncorrupted.
#
# Optionally gives the helper its own temporary directory, unique to this
# execution (TEMP_ROOT and TEMP_KIND):
#   TEMP_KIND=usable  creates "<TEMP_ROOT>/run <stamp>-<random> with spaces",
#                     passes it as the helper's second argument, and removes it
#                     afterwards
#   TEMP_KIND=missing points the temporary directory at a name that does not
#                     exist (and must still not exist afterwards)
# TMPDIR, TMP, TEMP and TEMPDIR are set for the helper only. Nothing outside
# TEMP_ROOT is ever created or removed; anything left behind, or a failed
# cleanup, fails the test and is listed in the message.
#
# Invoked by CTest as:
#   cmake -DEXE=<path> -DARGS=<arg> -DEXPECT_EXIT=<n> [-DMATCH_1=<regex> ...]
#         [-DREJECT_1=<regex> ...] [-DCONCURRENT_LINES=<n>]
#         [-DTEMP_ROOT=<dir> -DTEMP_KIND=usable|missing] -P RunCase.cmake

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED EXE OR NOT DEFINED EXPECT_EXIT)
   message(FATAL_ERROR "RunCase.cmake: EXE and EXPECT_EXIT are required")
endif()

# ---- per-execution temporary workspace --------------------------------------

set(tempDir "")
set(cleanupError "")

if(DEFINED TEMP_ROOT)
   # the only directories this script will create or delete live under a
   # "regression-tmp" directory that the build tree created for this test
   if(NOT IS_ABSOLUTE "${TEMP_ROOT}" OR NOT "${TEMP_ROOT}" MATCHES "/regression-tmp/[^/]+$")
      message(FATAL_ERROR "RunCase.cmake: unsafe TEMP_ROOT '${TEMP_ROOT}'")
   endif()

   string(TIMESTAMP stamp "%Y%m%d%H%M%S")
   string(RANDOM LENGTH 10 ALPHABET "0123456789abcdef" suffix)

   if("${TEMP_KIND}" STREQUAL "usable")
      set(tempDir "${TEMP_ROOT}/run ${stamp}-${suffix} with spaces")
      if(EXISTS "${tempDir}")
         message(FATAL_ERROR "RunCase.cmake: '${tempDir}' already exists")
      endif()
      file(MAKE_DIRECTORY "${tempDir}")
      set(ARG2 "${tempDir}")
   elseif("${TEMP_KIND}" STREQUAL "missing")
      set(tempDir "${TEMP_ROOT}/no such directory ${stamp}-${suffix}")
   else()
      message(FATAL_ERROR "RunCase.cmake: TEMP_KIND must be usable or missing")
   endif()

   # inherited by the helper started below
   set(ENV{TMPDIR} "${tempDir}")
   set(ENV{TMP} "${tempDir}")
   set(ENV{TEMP} "${tempDir}")
   set(ENV{TEMPDIR} "${tempDir}")
endif()

# ARG2 (optional) is a second argument that may contain spaces
set(extraArg "")
if(DEFINED ARG2)
   set(extraArg "${ARG2}")
endif()

execute_process(
   COMMAND "${EXE}" ${ARGS} ${extraArg}
   RESULT_VARIABLE rc
   OUTPUT_VARIABLE out
   ERROR_VARIABLE err
   TIMEOUT 120
)

set(all "${out}${err}")
string(REPLACE "\r" "" all "${all}")

# ---- cleanup (before any check, so a failing check cannot skip it) ----------

if(DEFINED TEMP_ROOT)
   if("${TEMP_KIND}" STREQUAL "usable")
      file(GLOB_RECURSE leftovers LIST_DIRECTORIES true "${tempDir}/*")
      file(REMOVE_RECURSE "${tempDir}")
      if(EXISTS "${tempDir}")
         set(cleanupError "could not remove the test directory '${tempDir}'")
      elseif(leftovers)
         set(cleanupError "the test left files behind in '${tempDir}': ${leftovers}")
      endif()
   elseif(EXISTS "${tempDir}")
      set(cleanupError "'${tempDir}' was expected not to exist but does")
      file(REMOVE_RECURSE "${tempDir}")
   endif()

   # remove this test's own (now empty) workspace as well
   file(GLOB remaining "${TEMP_ROOT}/*")
   if(NOT remaining)
      file(REMOVE_RECURSE "${TEMP_ROOT}")
   endif()
endif()

function(case_fail message)
   message("---- output of ${EXE} ${ARGS} ----\n${all}\n---- end of output ----")
   if(cleanupError)
      message("---- cleanup problem ----\n${cleanupError}")
   endif()
   message(FATAL_ERROR "${message}")
endfunction()

if(NOT "${rc}" STREQUAL "${EXPECT_EXIT}")
   case_fail("expected exit status ${EXPECT_EXIT}, got '${rc}'")
endif()

# a sanitizer report must never be hidden behind an expected nonzero exit
if(all MATCHES "ERROR: AddressSanitizer|ERROR: LeakSanitizer|WARNING: ThreadSanitizer|runtime error:")
   case_fail("a sanitizer reported a problem in ${EXE}")
endif()

foreach(i RANGE 1 20)
   if(DEFINED MATCH_${i})
      if(NOT all MATCHES "${MATCH_${i}}")
         case_fail("output does not match expected pattern: ${MATCH_${i}}")
      endif()
   endif()
   if(DEFINED REJECT_${i})
      if(all MATCHES "${REJECT_${i}}")
         case_fail("output unexpectedly matches pattern: ${REJECT_${i}}")
      endif()
   endif()
endforeach()

if(DEFINED CONCURRENT_LINES)
   set(expectedLine "*** failure: expected true, got false (concurrent)")
   string(REPLACE "\n" ";" lines "${all}")
   set(failureLines 0)
   foreach(line IN LISTS lines)
      if(line MATCHES "^[*][*][*]")
         math(EXPR failureLines "${failureLines} + 1")
         if(NOT line STREQUAL expectedLine)
            case_fail("corrupted or interleaved failure line: '${line}'")
         endif()
      endif()
   endforeach()
   if(NOT failureLines EQUAL CONCURRENT_LINES)
      case_fail("expected ${CONCURRENT_LINES} intact failure lines, found ${failureLines}")
   endif()
endif()

if(cleanupError)
   case_fail("temporary workspace problem: ${cleanupError}")
endif()

message("OK: exit ${rc} and output as expected")
