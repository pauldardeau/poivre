// Shared body of the consumer programs; the including file selects the include
// style. It exercises the parts of poivre's public API that matter to a user:
// a classic suite with every assertion kind, an expected exception, a temporary
// file, a self-registering test case, the registry, and the exit-code helpers.

namespace {

class ThrowsExpected : public poivre::Runnable {
public:
   void run() override {
      throw poivre::BasicException("expected");
   }
};

class ConsumerSuite : public poivre::TestSuite {
public:
   ConsumerSuite() : TestSuite("ConsumerSuite") {}

protected:
   void runTests() override {
      TEST_CASE("assertions");
      require(true, "require");
      requireFalse(false, "requireFalse");
      requireStringEquals("poivre", "poivre", "requireStringEquals");
      requireNonEmptyString("x", "requireNonEmptyString");
      requireException("BasicException", new ThrowsExpected(), "requireException");

      const std::string path = getTempFile();
      requireNonEmptyString(path, "getTempFile");
      require(deleteFile(path), "deleteFile");
   }
};

class RegisteredSuite : public poivre::TestSuite {
public:
   RegisteredSuite() : TestSuite("RegisteredSuite") {}

protected:
   void runTests() override {}
};

}

POIVRE_TEST_CASE(RegisteredSuite, registeredCaseRuns) {
   require(true, "a POIVRE_TEST_CASE runs");
}

int main() {
   ConsumerSuite suite;
   suite.run();

   poivre::TestRegistry& registry = poivre::TestRegistry::instance();
   registry.runAll();

   const bool classicOk = (suite.exitCode() == 0);
   const bool registryOk = (registry.exitCode() == 0) &&
                           (registry.getNumRegisteredTestCases() == 1);

   std::printf("consumer: classic=%s registry=%s\n",
               classicOk ? "ok" : "FAILED", registryOk ? "ok" : "FAILED");
   return (classicOk && registryOk) ? 0 : 1;
}
