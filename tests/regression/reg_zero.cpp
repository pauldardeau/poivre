// Regression helper: an executable with no registered test cases must not
// report success.

#include <cstdio>

#include "TestRegistry.h"

int main() {
   poivre::TestRegistry& registry = poivre::TestRegistry::instance();
   std::printf("REGISTERED=%d\n", registry.getNumRegisteredTestCases());
   std::fflush(stdout);
   return registry.runAllAndGetExitCode();
}
