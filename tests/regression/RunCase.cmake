# Runs one regression helper executable as a subprocess and checks that it
#   - exited with exactly EXPECT_EXIT (a crash/abort/timeout never matches,
#     because execute_process then reports a message instead of a number),
#   - printed every MATCH_<n> regular expression,
#   - printed no REJECT_<n> regular expression,
#   - and, if CONCURRENT_LINES is set, printed exactly that many failure
#     lines, each one complete and uncorrupted.
#
# Invoked by CTest as:
#   cmake -DEXE=<path> -DARGS=<arg> -DEXPECT_EXIT=<n> [-DMATCH_1=<regex> ...]
#         [-DREJECT_1=<regex> ...] [-DCONCURRENT_LINES=<n>] -P RunCase.cmake

cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED EXE OR NOT DEFINED EXPECT_EXIT)
   message(FATAL_ERROR "RunCase.cmake: EXE and EXPECT_EXIT are required")
endif()

execute_process(
   COMMAND "${EXE}" ${ARGS}
   RESULT_VARIABLE rc
   OUTPUT_VARIABLE out
   ERROR_VARIABLE err
   TIMEOUT 120
)

set(all "${out}${err}")
string(REPLACE "\r" "" all "${all}")

function(case_fail message)
   message("---- output of ${EXE} ${ARGS} ----\n${all}\n---- end of output ----")
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

message("OK: exit ${rc} and output as expected")
