// Regression helper: a normal, fully passing registered run must exit 0 and
// print no failure lines.

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
   return poivre::TestRegistry::instance().runAllAndGetExitCode();
}
