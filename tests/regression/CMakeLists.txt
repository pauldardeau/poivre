# Subprocess regression tests for Poivre's test execution and failure
# reporting (exit status, exception isolation, zero-test detection,
# thread-safe counting). Each helper executable runs in its own process so
# its exit status, and the fact that it survived, can be checked by
# RunCase.cmake; nothing here makes the main test process crash.

function(poivre_add_helper target)
   add_executable(${target} ${ARGN})
   target_link_libraries(${target} PRIVATE poivre)
endfunction()

poivre_add_helper(poivre_reg_failures_256 reg_failures.cpp)
target_compile_definitions(poivre_reg_failures_256 PRIVATE NUM_FAILURES=256)

poivre_add_helper(poivre_reg_failures_512 reg_failures.cpp)
target_compile_definitions(poivre_reg_failures_512 PRIVATE NUM_FAILURES=512)

poivre_add_helper(poivre_reg_zero reg_zero.cpp)
poivre_add_helper(poivre_reg_success reg_success.cpp)
poivre_add_helper(poivre_reg_exceptions reg_exceptions.cpp)
poivre_add_helper(poivre_reg_classic classic_cases.cpp)

# poivre_add_case(<name> <target> [ARGS <arg>] EXIT <n>
#                 [MATCH <regex>...] [REJECT <regex>...] [CONCURRENT_LINES <n>])
function(poivre_add_case name target)
   cmake_parse_arguments(C "" "ARGS;EXIT;CONCURRENT_LINES" "MATCH;REJECT" ${ARGN})

   set(command ${CMAKE_COMMAND}
      "-DEXE=$<TARGET_FILE:${target}>"
      "-DARGS=${C_ARGS}"
      "-DEXPECT_EXIT=${C_EXIT}")

   set(i 0)
   foreach(pattern IN LISTS C_MATCH)
      math(EXPR i "${i} + 1")
      list(APPEND command "-DMATCH_${i}=${pattern}")
   endforeach()

   set(i 0)
   foreach(pattern IN LISTS C_REJECT)
      math(EXPR i "${i} + 1")
      list(APPEND command "-DREJECT_${i}=${pattern}")
   endforeach()

   if(DEFINED C_CONCURRENT_LINES)
      list(APPEND command "-DCONCURRENT_LINES=${C_CONCURRENT_LINES}")
   endif()

   list(APPEND command -P "${CMAKE_CURRENT_SOURCE_DIR}/RunCase.cmake")

   add_test(NAME ${name} COMMAND ${command})
   set_tests_properties(${name} PROPERTIES TIMEOUT 180)
endfunction()

# ---- exit status ----------------------------------------------------------

# 256 and 512 failures are multiples of 256: an exit status built from the raw
# count would be 0. runAll() must still report the exact count.
poivre_add_case(regression_exit_256_failures poivre_reg_failures_256 EXIT 1
   MATCH "RAW_COUNT=256" "EXIT_CODE_HELPER=1" "256 executed, 256 failed")

poivre_add_case(regression_exit_512_failures poivre_reg_failures_512 EXIT 1
   MATCH "RAW_COUNT=512" "EXIT_CODE_HELPER=1" "512 executed, 512 failed")

poivre_add_case(regression_zero_registered_tests poivre_reg_zero EXIT 1
   MATCH "0 executed, 0 failed" "0 test case[(]s[)] run"
         "no test cases were executed")

poivre_add_case(regression_success_registered poivre_reg_success EXIT 0
   MATCH "4 executed, 0 failed" "2 test case[(]s[)] run"
   REJECT "[*][*][*]")

# ---- exceptions in registered test cases ----------------------------------

poivre_add_case(regression_registered_exceptions poivre_reg_exceptions EXIT 1
   MATCH
      "unexpected exception in AThrowsSuite[.]throwsStdException: boom from a registered test"
      "unexpected exception in AThrowsSuite[.]throwsNonStdException: [(]not derived from std::exception[)]"
      "MARKER_RAN_AFTER_STD_THROW"
      "MARKER_RAN_AFTER_UNKNOWN_THROW"
      "unexpected exception in SetupThrowsSuite[.]bodyMustNotRun: setup failed"
      "MARKER_BODY_BEFORE_TEARDOWN_THROW"
      "unexpected exception in tearDown[(][)] of test case bodyRunsThenTearDownThrows: tearDown failed"
      "unexpected exception in BodyAndTearDownThrowSuite[.]bothThrow: body failed"
      "unexpected exception in tearDown[(][)] of test case bothThrow: tearDown failed while body was unwinding"
      "MARKER_LAST_TEST_RAN"
      "RAW_COUNT=6"
   REJECT "BODY_AFTER_SETUP_THROW_SHOULD_NOT_APPEAR")

# ---- exceptions in classic suites -----------------------------------------

poivre_add_case(regression_classic_success poivre_reg_classic ARGS success EXIT 0
   MATCH "SUITE_EXIT_CODE=0 FOLLOWER_EXIT_CODE=0" "MARKER_FOLLOWER_SUITE_RAN"
   REJECT "[*][*][*]")

poivre_add_case(regression_classic_body_throws_std poivre_reg_classic
   ARGS body_throws_std EXIT 1
   MATCH "unexpected exception in runTests[(][)] of suite ScenarioSuite: body failed"
         "MARKER_SUITE_TEARDOWN_RAN" "MARKER_FOLLOWER_SUITE_RAN"
         "SUITE_EXIT_CODE=1 FOLLOWER_EXIT_CODE=0")

poivre_add_case(regression_classic_body_throws_non_std poivre_reg_classic
   ARGS body_throws_non_std EXIT 1
   MATCH "unexpected exception in runTests[(][)] of suite ScenarioSuite: [(]not derived from std::exception[)]"
         "MARKER_SUITE_TEARDOWN_RAN" "MARKER_FOLLOWER_SUITE_RAN")

poivre_add_case(regression_classic_teardown_throws poivre_reg_classic
   ARGS teardown_throws EXIT 1
   MATCH "unexpected exception in tearDown[(][)] of test case one: tearDown failed"
         "unexpected exception in tearDown[(][)] of test case two: tearDown failed"
         "MARKER_SECOND_TEST_RAN" "MARKER_FOLLOWER_SUITE_RAN")

poivre_add_case(regression_classic_body_and_teardown_throw poivre_reg_classic
   ARGS body_and_teardown_throw EXIT 1
   MATCH "unexpected exception in tearDown[(][)] of test case one: tearDown failed"
         "unexpected exception in runTests[(][)] of suite ScenarioSuite: body failed"
         "MARKER_FOLLOWER_SUITE_RAN")

poivre_add_case(regression_classic_suite_setup_throws poivre_reg_classic
   ARGS suite_setup_throws EXIT 1
   MATCH "unexpected exception in suiteSetup[(][)] of suite ScenarioSuite: suiteSetup failed"
         "MARKER_SUITE_TEARDOWN_RAN" "MARKER_FOLLOWER_SUITE_RAN"
   REJECT "MARKER_RUN_TESTS_REACHED")

poivre_add_case(regression_classic_suite_teardown_throws poivre_reg_classic
   ARGS suite_teardown_throws EXIT 1
   MATCH "unexpected exception in suiteTearDown[(][)] of suite ScenarioSuite: suiteTearDown failed"
         "MARKER_SECOND_TEST_RAN" "MARKER_FOLLOWER_SUITE_RAN")

poivre_add_case(regression_classic_no_assertions poivre_reg_classic
   ARGS no_assertions EXIT 1
   MATCH "warning: suite ScenarioSuite evaluated no assertions"
         "SUITE_EXIT_CODE=1 FOLLOWER_EXIT_CODE=0")

# ---- thread-safe accounting -----------------------------------------------

# 8 threads x 2000 assertions, every 10th failing: exact totals, and every one
# of the 1600 failure lines printed intact
poivre_add_case(regression_concurrent_assertions poivre_reg_classic
   ARGS concurrent EXIT 0
   MATCH "ASSERTIONS=16000 FAILURES=1600 COUNTS_OK"
   CONCURRENT_LINES 1600)
