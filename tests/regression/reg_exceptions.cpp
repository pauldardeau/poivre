// Regression helper: exceptions escaping registered test cases, setup() and
// tearDown() must be recorded as failures, must not terminate the process,
// and must not stop later test cases from running.
//
// Expected failures: std exception 1 + non-std exception 1 + setup throws 1 +
// tearDown throws 1 + body and tearDown both throw 2 = 6.

#include <cstdio>
#include <stdexcept>

#include "TestMacros.h"
#include "TestRegistry.h"
#include "TestSuite.h"

namespace {

class PlainSuite : public poivre::TestSuite {
public:
   PlainSuite() : TestSuite("PlainSuite") {}

protected:
   void runTests() override {}
};

class SetupThrowsSuite : public poivre::TestSuite {
public:
   SetupThrowsSuite() : TestSuite("BSetupThrows") {}

protected:
   void setup() override {
      throw std::runtime_error("setup failed");
   }
   void runTests() override {}
};

class TearDownThrowsSuite : public poivre::TestSuite {
public:
   TearDownThrowsSuite() : TestSuite("CTearDownThrows") {}

protected:
   void tearDown() override {
      throw std::runtime_error("tearDown failed");
   }
   void runTests() override {}
};

class BodyAndTearDownThrowSuite : public poivre::TestSuite {
public:
   BodyAndTearDownThrowSuite() : TestSuite("DBodyAndTearDownThrow") {}

protected:
   void tearDown() override {
      throw std::runtime_error("tearDown failed while body was unwinding");
   }
   void runTests() override {}
};

}

// the registry runs suites sorted by fixture class name (AThrowsSuite,
// BodyAndTearDownThrowSuite, SetupThrowsSuite, TearDownThrowsSuite,
// ZLastSuiteBase), so the final suite proves the run continued past every
// earlier problem

class AThrowsSuite : public PlainSuite {};

POIVRE_TEST_CASE(AThrowsSuite, throwsStdException) {
   throw std::runtime_error("boom from a registered test");
}

POIVRE_TEST_CASE(AThrowsSuite, runsAfterStdException) {
   std::printf("MARKER_RAN_AFTER_STD_THROW\n");
   require(true, "this test case must still run");
}

POIVRE_TEST_CASE(AThrowsSuite, throwsNonStdException) {
   throw 42;
}

POIVRE_TEST_CASE(AThrowsSuite, runsAfterNonStdException) {
   std::printf("MARKER_RAN_AFTER_UNKNOWN_THROW\n");
   require(true, "this test case must still run");
}

POIVRE_TEST_CASE(SetupThrowsSuite, bodyMustNotRun) {
   std::printf("MARKER_BODY_AFTER_SETUP_THROW_SHOULD_NOT_APPEAR\n");
   require(true, "unreachable");
}

POIVRE_TEST_CASE(TearDownThrowsSuite, bodyRunsThenTearDownThrows) {
   std::printf("MARKER_BODY_BEFORE_TEARDOWN_THROW\n");
   require(true, "body is fine");
}

POIVRE_TEST_CASE(BodyAndTearDownThrowSuite, bothThrow) {
   throw std::runtime_error("body failed");
}

namespace {
class ZLastSuiteBase : public poivre::TestSuite {
public:
   ZLastSuiteBase() : TestSuite("ZLast") {}
protected:
   void runTests() override {}
};
}

POIVRE_TEST_CASE(ZLastSuiteBase, lastTestStillRuns) {
   std::printf("MARKER_LAST_TEST_RAN\n");
   require(true, "the final test case ran");
}

int main() {
   poivre::TestRegistry& registry = poivre::TestRegistry::instance();
   const int rawCount = registry.runAll();
   std::printf("RAW_COUNT=%d\n", rawCount);
   std::fflush(stdout);
   return registry.exitCode();
}
