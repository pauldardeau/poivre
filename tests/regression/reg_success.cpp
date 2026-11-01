// Regression helper: a normal, fully passing registered run must exit 0 and
// print no failure lines.

#include <cstdio>

#include "TestMacros.h"
#include "TestRegistry.h"
#include "TestSuite.h"

namespace {

class PassingSuite : public poivre::TestSuite {
public:
   PassingSuite() : TestSuite("PassingSuite") {}

protected:
   void runTests() override {}
};

}

POIVRE_TEST_CASE(PassingSuite, firstPasses) {
   require(true, "first");
   requireFalse(false, "second");
}

POIVRE_TEST_CASE(PassingSuite, secondPasses) {
   requireStringEquals("a", "a", "strings");
   requireNonEmptyString("x", "non-empty");
}

int main() {
   poivre::TestRegistry& registry = poivre::TestRegistry::instance();
   std::printf("REGISTERED=%d\n", registry.getNumRegisteredTestCases());
   std::fflush(stdout);
   return registry.runAllAndGetExitCode();
}
