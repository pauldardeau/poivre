// Regression helper: an executable with no registered test cases must not
// report success.

#include "TestRegistry.h"

int main() {
   return poivre::TestRegistry::instance().runAllAndGetExitCode();
}
