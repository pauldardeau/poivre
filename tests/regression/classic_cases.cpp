// Regression helper for classic (hand-written runTests()) suites. The first
// command-line argument selects the scenario; each scenario runs in its own
// process so exit status and process survival can be checked by
// RunCase.cmake. See tests/regression/CMakeLists.txt for expectations.

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

#include "TestMacros.h"
#include "TestSuite.h"

namespace {

struct Scenario {
   bool throwInSuiteSetup = false;
   bool throwStdInBody = false;
   bool throwNonStdInBody = false;
   bool throwInTearDown = false;
   bool throwInSuiteTearDown = false;
   bool assertNothing = false;
};

class ScenarioSuite : public poivre::TestSuite {
public:
   explicit ScenarioSuite(const Scenario& scenario) :
      TestSuite("ScenarioSuite"),
      m_scenario(scenario) {
   }

protected:
   void suiteSetup() override {
      if (m_scenario.throwInSuiteSetup) {
         throw std::runtime_error("suiteSetup failed");
      }
   }

   void tearDown() override {
      if (m_scenario.throwInTearDown) {
         throw std::runtime_error("tearDown failed");
      }
   }

   void suiteTearDown() override {
      std::printf("MARKER_SUITE_TEARDOWN_RAN\n");
      if (m_scenario.throwInSuiteTearDown) {
         throw std::runtime_error("suiteTearDown failed");
      }
   }

   void runTests() override {
      std::printf("MARKER_RUN_TESTS_REACHED\n");
      testOne();
      testTwo();
   }

private:
   void testOne() {
      TEST_CASE("one");
      if (m_scenario.throwStdInBody) {
         throw std::runtime_error("body failed");
      }
      if (m_scenario.throwNonStdInBody) {
         throw 42;
      }
      if (!m_scenario.assertNothing) {
         require(true, "first test passes");
      }
   }

   void testTwo() {
      TEST_CASE("two");
      std::printf("MARKER_SECOND_TEST_RAN\n");
      if (!m_scenario.assertNothing) {
         require(true, "second test passes");
      }
   }

   Scenario m_scenario;
};

// proves the process survived and a later suite still runs
class FollowerSuite : public poivre::TestSuite {
public:
   FollowerSuite() : TestSuite("FollowerSuite") {}

protected:
   void runTests() override {
      TEST_CASE("follower");
      std::printf("MARKER_FOLLOWER_SUITE_RAN\n");
      require(true, "follower passes");
   }
};

constexpr int kThreads = 8;
constexpr int kAssertionsPerThread = 2000;

// every 10th assertion fails: 200 failures per thread, 1600 in total
class ConcurrentSuite : public poivre::TestSuite {
public:
   ConcurrentSuite() : TestSuite("ConcurrentSuite") {}

protected:
   void runTests() override {
      TEST_CASE("concurrent assertions");
      std::vector<std::thread> threads;
      for (int t = 0; t < kThreads; ++t) {
         threads.emplace_back([this] {
            for (int i = 0; i < kAssertionsPerThread; ++i) {
               require((i % 10) != 0, "concurrent");
            }
         });
      }
      for (auto& th : threads) {
         th.join();
      }
   }
};

int runScenario(const Scenario& scenario) {
   ScenarioSuite suite(scenario);
   suite.run();
   FollowerSuite follower;
   follower.run();
   std::printf("SUITE_EXIT_CODE=%d FOLLOWER_EXIT_CODE=%d\n",
               suite.exitCode(), follower.exitCode());
   std::fflush(stdout);
   return (suite.exitCode() != 0 || follower.exitCode() != 0) ? 1 : 0;
}

}

int main(int argc, char** argv) {
   const char* name = (argc > 1) ? argv[1] : "";
   Scenario s;

   if (std::strcmp(name, "success") == 0) {
      return runScenario(s);
   } else if (std::strcmp(name, "body_throws_std") == 0) {
      s.throwStdInBody = true;
      return runScenario(s);
   } else if (std::strcmp(name, "body_throws_non_std") == 0) {
      s.throwNonStdInBody = true;
      return runScenario(s);
   } else if (std::strcmp(name, "teardown_throws") == 0) {
      s.throwInTearDown = true;
      return runScenario(s);
   } else if (std::strcmp(name, "body_and_teardown_throw") == 0) {
      s.throwStdInBody = true;
      s.throwInTearDown = true;
      return runScenario(s);
   } else if (std::strcmp(name, "suite_setup_throws") == 0) {
      s.throwInSuiteSetup = true;
      return runScenario(s);
   } else if (std::strcmp(name, "suite_teardown_throws") == 0) {
      s.throwInSuiteTearDown = true;
      return runScenario(s);
   } else if (std::strcmp(name, "no_assertions") == 0) {
      s.assertNothing = true;
      return runScenario(s);
   } else if (std::strcmp(name, "concurrent") == 0) {
      ConcurrentSuite suite;
      suite.run();
      const int expectedAssertions = kThreads * kAssertionsPerThread;
      const int expectedFailures = kThreads * (kAssertionsPerThread / 10);
      const bool countsOk =
         suite.getNumTestsExecuted() == expectedAssertions &&
         suite.getNumFailures() == expectedFailures;
      std::printf("ASSERTIONS=%d FAILURES=%d %s\n",
                  suite.getNumTestsExecuted(),
                  suite.getNumFailures(),
                  countsOk ? "COUNTS_OK" : "COUNTS_WRONG");
      std::fflush(stdout);
      return countsOk ? 0 : 2;
   }

   std::printf("unknown scenario '%s'\n", name);
   return 3;
}
