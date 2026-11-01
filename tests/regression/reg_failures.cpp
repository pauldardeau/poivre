// Regression helper: a registered test case with exactly NUM_FAILURES failing
// assertions (compiled twice, for 256 and 512). The exit status must be 1
// even though 256 and 512 are multiples of 256 (an exit status is truncated
// to 8 bits on POSIX), while runAll() must still report the exact count.

#include <cstdio>

#include "TestMacros.h"
#include "TestRegistry.h"
#include "TestSuite.h"

#ifndef NUM_FAILURES
#error NUM_FAILURES must be defined
#endif

namespace {

class ManyFailuresSuite : public poivre::TestSuite {
public:
   ManyFailuresSuite() : TestSuite("ManyFailuresSuite") {}

protected:
   void runTests() override {}
};

}

POIVRE_TEST_CASE(ManyFailuresSuite, failsManyTimes) {
   for (int i = 0; i < NUM_FAILURES; ++i) {
      require(false, "intentional failure");
   }
}

int main() {
   poivre::TestRegistry& registry = poivre::TestRegistry::instance();
   const int rawCount = registry.runAll();
   std::printf("RAW_COUNT=%d\n", rawCount);
   std::printf("EXIT_CODE_HELPER=%d\n", registry.exitCode());
   std::fflush(stdout);
   return registry.exitCode();
}
