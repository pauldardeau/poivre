// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#include <algorithm>
#include <cstdio>
#include <exception>

#include "TestRegistry.h"

using namespace poivre;

//******************************************************************************

TestRegistry& TestRegistry::instance() {
   static TestRegistry registry;
   return registry;
}

//******************************************************************************

void TestRegistry::addTestCase(const std::string& suiteName,
                               const std::string& testName,
                               TestCaseFn fn) {
   m_entries.push_back({suiteName, testName, fn});
}

//******************************************************************************

void TestRegistry::recordResult(int numTestsExecuted, int numFailures) {
   m_totalTestCasesRun += 1;
   m_totalTestsExecuted += numTestsExecuted;
   m_totalFailures += numFailures;
}

//******************************************************************************

int TestRegistry::exitCodeFor(int numTestCases,
                              int numAssertions,
                              int numFailures) {
   return (numTestCases > 0 && numAssertions > 0 && numFailures == 0) ? 0 : 1;
}

//******************************************************************************

int TestRegistry::exitCode() const {
   return exitCodeFor(m_totalTestCasesRun,
                      m_totalTestsExecuted,
                      m_totalFailures);
}

//******************************************************************************

int TestRegistry::getNumTestCasesRun() const {
   return m_totalTestCasesRun;
}

//******************************************************************************

int TestRegistry::getNumAssertions() const {
   return m_totalTestsExecuted;
}

//******************************************************************************

int TestRegistry::getNumFailures() const {
   return m_totalFailures;
}

//******************************************************************************

int TestRegistry::getNumRegisteredTestCases() const {
   return static_cast<int>(m_entries.size());
}

//******************************************************************************

int TestRegistry::runAll() {
   m_totalTestCasesRun = 0;
   m_totalTestsExecuted = 0;
   m_totalFailures = 0;

   // group by suite name so the "running suite" banner prints once per
   // suite, regardless of what order static initializers across
   // different translation units happened to run in. stable_sort
   // preserves each suite's own registration (declaration) order.
   std::vector<Entry> ordered = m_entries;
   std::stable_sort(ordered.begin(), ordered.end(),
                    [](const Entry& a, const Entry& b) {
                       return a.suiteName < b.suiteName;
                    });

   std::string lastSuiteName;

   for (const auto& entry : ordered) {
      if (entry.suiteName != lastSuiteName) {
         printf("+++++++++++++ running suite: %s +++++++++++\n",
                entry.suiteName.c_str());
         fflush(stdout);
         lastSuiteName = entry.suiteName;
      }

      // POIVRE_TEST_CASE catches exceptions from the test body, setup()
      // and tearDown() itself; this is the backstop for anything that
      // escapes it (e.g. a fixture whose constructor throws) so one bad
      // test case cannot take the rest of the run down with it
      try {
         entry.fn();
      } catch (const std::exception& e) {
         printf("*** failure: unexpected exception escaped test case %s.%s: %s\n",
                entry.suiteName.c_str(),
                entry.testName.c_str(),
                e.what());
         fflush(stdout);
         recordResult(1, 1);
      } catch (...) {
         printf("*** failure: unexpected exception escaped test case %s.%s: "
                "(not derived from std::exception)\n",
                entry.suiteName.c_str(),
                entry.testName.c_str());
         fflush(stdout);
         recordResult(1, 1);
      }
   }

   const int testCasesRun = m_totalTestCasesRun;
   const int assertions = m_totalTestsExecuted;
   const int failures = m_totalFailures;

   printf("\n=============================================\n");
   printf("%d executed, %d failed\n", assertions, failures);
   printf("%d test case(s) run\n", testCasesRun);
   if (testCasesRun == 0) {
      printf("*** no test cases were executed; exitCode() reports failure\n");
   } else if (assertions == 0) {
      printf("*** no assertions were evaluated; exitCode() reports failure\n");
   }
   printf("=============================================\n");
   fflush(stdout);

   return failures;
}

//******************************************************************************

int TestRegistry::runAllAndGetExitCode() {
   runAll();
   return exitCode();
}

//******************************************************************************

AutoRegisterTestCase::AutoRegisterTestCase(const std::string& suiteName,
                                           const std::string& testName,
                                           TestRegistry::TestCaseFn fn) {
   TestRegistry::instance().addTestCase(suiteName, testName, fn);
}
