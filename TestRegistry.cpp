// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#include <algorithm>
#include <cstdio>

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
   m_totalTestsExecuted += numTestsExecuted;
   m_totalFailures += numFailures;
}

//******************************************************************************

int TestRegistry::runAll() {
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

      entry.fn();
   }

   printf("\n=============================================\n");
   printf("%d executed, %d failed\n", m_totalTestsExecuted, m_totalFailures);
   printf("=============================================\n");
   fflush(stdout);

   return m_totalFailures;
}

//******************************************************************************

AutoRegisterTestCase::AutoRegisterTestCase(const std::string& suiteName,
                                           const std::string& testName,
                                           TestRegistry::TestCaseFn fn) {
   TestRegistry::instance().addTestCase(suiteName, testName, fn);
}
